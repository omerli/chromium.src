# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import platform
import shutil
import socket
import sys
import tempfile
import time
import urllib2
import zipfile

from telemetry.page import profile_creator

import page_sets

from telemetry import benchmark
from telemetry.page import page_test
from telemetry.page import test_expectations
from telemetry.results import results_options
from telemetry.user_story import user_story_runner

class _ExtensionPageTest(page_test.PageTest):
  """This page test verified that extensions were automatically installed."""
  def __init__(self):
    super(_ExtensionPageTest, self).__init__()
    self._page_set = page_sets.Typical25PageSet()

    # Have the extensions been installed yet?
    self._extensions_installed = False

    # Expected
    self._expected_extension_count = 0

  def CanRunForPage(self, page):
    # Superclass override.
    # No matter how many pages in the pageset, just perform two test
    # iterations.
    return page.page_set.pages.index(page) < 2

  def ValidateAndMeasurePage(self, _, tab, results):
    # Superclass override.
    # Profile setup works in 2 phases:
    # Phase 1: When the first page is loaded: we wait for a timeout to allow
    #     all extensions to install and to prime safe browsing and other
    #     caches.  Extensions may open tabs as part of the install process.
    # Phase 2: When the second page loads, user_story_runner closes all tabs -
    #     we are left with one open tab, wait for that to finish loading.

    # Sleep for a bit to allow safe browsing and other data to load +
    # extensions to install.
    if not self._extensions_installed:
      sleep_seconds = 5 * 60
      logging.info("Sleeping for %d seconds." % sleep_seconds)
      time.sleep(sleep_seconds)
      self._extensions_installed = True
    else:
      # Phase 2: Wait for tab to finish loading.
      for i in xrange(len(tab.browser.tabs)):
        t = tab.browser.tabs[i]
        t.WaitForDocumentReadyStateToBeComplete()

  def DidRunTest(self, browser, results):
    """Superclass override."""
    super(_ExtensionPageTest, self).DidRunTest(browser,
        results)
    # Do some basic sanity checks to make sure the profile is complete.
    installed_extensions = browser.extensions.keys()
    if not len(installed_extensions) == self._expected_extension_count:
      # Diagnosing errors:
      # Too many extensions: Managed environment may be installing additional
      # extensions.
      raise Exception("Unexpected number of extensions installed in browser",
          installed_extensions)


def _ExternalExtensionsPath():
  """Returns the OS-dependent path at which to install the extension deployment
   files"""
  if platform.system() == 'Darwin':
    return os.path.join('/Library', 'Application Support', 'Google', 'Chrome',
        'External Extensions')
  elif platform.system() == 'Linux':
    return os.path.join('/opt', 'google', 'chrome', 'extensions' )
  else:
    raise NotImplementedError('Extension install on %s is not yet supported' %
        platform.system())


def _DownloadExtension(extension_id, output_dir):
  """Download an extension to disk.

  Args:
    extension_id: the extension id.
    output_dir: Directory to download into.

  Returns:
    Extension file downloaded."""
  extension_download_path = os.path.join(output_dir, "%s.crx" % extension_id)
  extension_url = (
      "https://clients2.google.com/service/update2/crx?response=redirect"
      "&x=id%%3D%s%%26lang%%3Den-US%%26uc" % extension_id)
  response = urllib2.urlopen(extension_url)
  assert(response.getcode() == 200)

  with open(extension_download_path, "w") as f:
    f.write(response.read())

  return extension_download_path


def _GetExtensionInfoFromCRX(crx_path):
  """Parse an extension archive and return information.

  Note:
    The extension name returned by this function may not be valid
  (e.g. in the case of a localized extension name).  It's use is just
  meant to be informational.

  Args:
    crx_path: path to crx archive to look at.

  Returns:
    Tuple consisting of:
    (crx_version, extension_name)"""
  crx_zip = zipfile.ZipFile(crx_path)
  manifest_contents = crx_zip.read('manifest.json')
  decoded_manifest = json.loads(manifest_contents)
  crx_version = decoded_manifest['version']
  extension_name = decoded_manifest['name']

  return (crx_version, extension_name)


class ExtensionsProfileCreator(profile_creator.ProfileCreator):
  """Abstract base class for profile creators that install extensions.

  Extensions are installed using the mechanism described in
  https://developer.chrome.com/extensions/external_extensions.html .

  Subclasses are meant to be run interactively.
  """
  def __init__(self, extensions_to_install=None, theme_to_install=None):
    self._CheckTestEnvironment()
    super(ExtensionsProfileCreator, self).__init__()

    # List of extensions to install.
    self._extensions_to_install = list(extensions_to_install or [])

    # Theme to install (if any).
    self._theme_to_install = theme_to_install

    # Directory to download extension files into.
    self._extension_download_dir = None

    # List of files to delete after run.
    self._files_to_cleanup = []

  def _CheckTestEnvironment(self):
    # Running this script on a corporate network or other managed environment
    # could potentially alter the profile contents.
    hostname = socket.gethostname()
    if hostname.endswith('corp.google.com'):
      raise Exception("It appears you are connected to a corporate network "
          "(hostname=%s).  This script needs to be run off the corp "
          "network." % hostname)

    prompt = ("\n!!!This script must be run on a fresh OS installation, "
        "disconnected from any corporate network. Are you sure you want to "
        "continue? (y/N) ")
    if (raw_input(prompt).lower() != 'y'):
      sys.exit(-1)

  def Run(self, options):
    self._PrepareExtensionInstallFiles()

    expectations = test_expectations.TestExpectations()
    results = results_options.CreateResults(
        benchmark.BenchmarkMetadata(profile_creator.__class__.__name__),
        options)
    extension_page_test = _ExtensionPageTest()
    extension_page_test._expected_extension_count = len(
        self._extensions_to_install)
    user_story_runner.Run(extension_page_test, extension_page_test._page_set,
        expectations, options, results)

    self._CleanupExtensionInstallFiles()

    # Check that files on this list exist and have content.
    expected_files = [
        os.path.join('Default', 'Network Action Predictor')]
    for filename in expected_files:
      filename = os.path.join(options.output_profile_path, filename)
      if not os.path.getsize(filename) > 0:
        raise Exception("Profile not complete: %s is zero length." % filename)

    if results.failures:
      logging.warning('Some pages failed.')
      logging.warning('Failed pages:\n%s',
                      '\n'.join(map(str, results.pages_that_failed)))
      raise Exception('ExtensionsProfileCreator failed.')

  def _PrepareExtensionInstallFiles(self):
    """Download extension archives and create extension install files."""
    extensions_to_install = self._extensions_to_install
    if self._theme_to_install:
      extensions_to_install.append(self._theme_to_install)
    if not extensions_to_install:
      raise ValueError("No extensions or themes to install:",
          extensions_to_install)

    # Create external extensions path if it doesn't exist already.
    external_extensions_dir = _ExternalExtensionsPath()
    if not os.path.isdir(external_extensions_dir):
      os.makedirs(external_extensions_dir)

    self._extension_download_dir = tempfile.mkdtemp()

    num_extensions = len(extensions_to_install)
    for i, extension_id in extensions_to_install:
      logging.info("Downloading %s - %d/%d" % (
          extension_id, (i + 1), num_extensions))
      extension_path = _DownloadExtension(extension_id,
          self._extension_download_dir)
      (version, name) = _GetExtensionInfoFromCRX(extension_path)
      extension_info = {'external_crx' : extension_path,
          'external_version' : version,
          '_comment' : name}
      extension_json_path = os.path.join(external_extensions_dir,
          "%s.json" % extension_id)
      with open(extension_json_path, 'w') as f:
        f.write(json.dumps(extension_info))
        self._files_to_cleanup.append(extension_json_path)

  def _CleanupExtensionInstallFiles(self):
    """Cleanup stray files before exiting."""
    logging.info("Cleaning up stray files")
    for filename in self._files_to_cleanup:
      os.remove(filename)

    if self._extension_download_dir:
      # Simple sanity check to lessen the impact of a stray rmtree().
      if len(self._extension_download_dir.split(os.sep)) < 3:
        raise Exception("Path too shallow: %s" % self._extension_download_dir)
      shutil.rmtree(self._extension_download_dir)
      self._extension_download_dir = None
