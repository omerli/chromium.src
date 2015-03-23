// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/sandboxed_zip_analyzer.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/safe_browsing/zip_analyzer_results.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

class SandboxedZipAnalyzerTest : public ::testing::Test {
 protected:
  // Constants for validating the data reported by the analyzer.
  struct BinaryData {
    const char* file_basename;
    ClientDownloadRequest_DownloadType download_type;
    const uint8_t* sha256_digest;
    int64_t length;
  };

  // A helper that provides a SandboxedZipAnalyzer::ResultCallback that will
  // store a copy of an analyzer's results and then run a closure.
  class ResultsGetter {
   public:
    ResultsGetter(const base::Closure& quit_closure,
                  zip_analyzer::Results* results)
        : quit_closure_(quit_closure),
          results_(results) {
      DCHECK(results);
      results->success = false;
    }

    SandboxedZipAnalyzer::ResultCallback GetCallback() {
      return base::Bind(&ResultsGetter::OnZipAnalyzerResults,
                        base::Unretained(this));
    }

   private:
    void OnZipAnalyzerResults(const zip_analyzer::Results& results) {
      *results_ = results;
      quit_closure_.Run();
    }

    base::Closure quit_closure_;
    zip_analyzer::Results* results_;
    DISALLOW_COPY_AND_ASSIGN(ResultsGetter);
  };

  SandboxedZipAnalyzerTest()
      : browser_thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &dir_test_data_));
    dir_test_data_ = dir_test_data_.AppendASCII("safe_browsing");
    dir_test_data_ = dir_test_data_.AppendASCII("download_protection");
  }

  // Runs a sandboxed zip analyzer on |file_path|, writing its results into
  // |results|.
  void RunAnalyzer(const base::FilePath& file_path,
                   zip_analyzer::Results* results) {
    DCHECK(results);
    base::RunLoop run_loop;
    ResultsGetter results_getter(run_loop.QuitClosure(), results);
    scoped_refptr<SandboxedZipAnalyzer> analyzer(
        new SandboxedZipAnalyzer(file_path, results_getter.GetCallback()));
    analyzer->Start();
    run_loop.Run();
  }

  // Verifies expectations about a binary found by the analyzer.
  void ExpectBinary(const BinaryData& data,
                    const ClientDownloadRequest_ArchivedBinary& binary) {
    ASSERT_TRUE(binary.has_file_basename());
    EXPECT_EQ(data.file_basename, binary.file_basename());
    ASSERT_TRUE(binary.has_download_type());
    EXPECT_EQ(data.download_type, binary.download_type());
    ASSERT_TRUE(binary.has_digests());
    ASSERT_TRUE(binary.digests().has_sha256());
    EXPECT_EQ(std::string(data.sha256_digest,
                          data.sha256_digest + crypto::kSHA256Length),
              binary.digests().sha256());
    EXPECT_FALSE(binary.digests().has_sha1());
    EXPECT_FALSE(binary.digests().has_md5());
    ASSERT_TRUE(binary.has_length());
    EXPECT_EQ(data.length, binary.length());
    EXPECT_FALSE(binary.has_signature());
#if defined(OS_WIN)  // ExtractImageHeaders is only implemented for Win.
    ASSERT_TRUE(binary.has_image_headers());
    ASSERT_TRUE(binary.image_headers().has_pe_headers());
    EXPECT_TRUE(binary.image_headers().pe_headers().has_dos_header());
    EXPECT_TRUE(binary.image_headers().pe_headers().has_file_header());
    EXPECT_TRUE(binary.image_headers().pe_headers().has_optional_headers32());
    EXPECT_FALSE(binary.image_headers().pe_headers().has_optional_headers64());
#else  // OS_WIN
    ASSERT_FALSE(binary.has_image_headers());
#endif  // !OS_WIN
  }

  static const uint8_t kUnsignedDigest[];
  static const uint8_t kSignedDigest[];
  static const BinaryData kUnsignedExe;
  static const BinaryData kSignedExe;

  base::FilePath dir_test_data_;
  content::TestBrowserThreadBundle browser_thread_bundle_;
  content::InProcessUtilityThreadHelper utility_thread_helper_;
};

// static
const uint8_t SandboxedZipAnalyzerTest::kUnsignedDigest[] = {
  0x1e, 0x95, 0x4d, 0x9c, 0xe0, 0x38, 0x9e, 0x2b, 0xa7, 0x44, 0x72, 0x16,
  0xf2, 0x17, 0x61, 0xf9, 0x8d, 0x1e, 0x65, 0x40, 0xc2, 0xab, 0xec, 0xdb,
  0xec, 0xff, 0x57, 0x0e, 0x36, 0xc4, 0x93, 0xdb
};
const uint8_t SandboxedZipAnalyzerTest::kSignedDigest[] = {
  0xe1, 0x1f, 0xfa, 0x0c, 0x9f, 0x25, 0x23, 0x44, 0x53, 0xa9, 0xed, 0xd1,
  0xcb, 0x25, 0x1d, 0x46, 0x10, 0x7f, 0x34, 0xb5, 0x36, 0xad, 0x74, 0x64,
  0x2a, 0x85, 0x84, 0xac, 0xa8, 0xc1, 0xa8, 0xce
};
const SandboxedZipAnalyzerTest::BinaryData
    SandboxedZipAnalyzerTest::kUnsignedExe = {
  "unsigned.exe",
  ClientDownloadRequest_DownloadType_WIN_EXECUTABLE,
  &kUnsignedDigest[0],
  36864,
};
const SandboxedZipAnalyzerTest::BinaryData
    SandboxedZipAnalyzerTest::kSignedExe = {
  "signed.exe",
  ClientDownloadRequest_DownloadType_WIN_EXECUTABLE,
  &kSignedDigest[0],
  37768,
};

TEST_F(SandboxedZipAnalyzerTest, NoBinaries) {
  zip_analyzer::Results results;
  RunAnalyzer(dir_test_data_.AppendASCII("zipfile_no_binaries.zip"), &results);
  ASSERT_TRUE(results.success);
  EXPECT_FALSE(results.has_executable);
  EXPECT_FALSE(results.has_archive);
  EXPECT_EQ(0, results.archived_binary.size());
}

TEST_F(SandboxedZipAnalyzerTest, OneUnsignedBinary) {
  zip_analyzer::Results results;
  RunAnalyzer(dir_test_data_.AppendASCII("zipfile_one_unsigned_binary.zip"),
              &results);
  ASSERT_TRUE(results.success);
  EXPECT_TRUE(results.has_executable);
  EXPECT_FALSE(results.has_archive);
  ASSERT_EQ(1, results.archived_binary.size());
  ExpectBinary(kUnsignedExe, results.archived_binary.Get(0));
}

TEST_F(SandboxedZipAnalyzerTest, TwoBinariesOneSigned) {
  zip_analyzer::Results results;
  RunAnalyzer(dir_test_data_.AppendASCII("zipfile_two_binaries_one_signed.zip"),
              &results);
  ASSERT_TRUE(results.success);
  EXPECT_TRUE(results.has_executable);
  EXPECT_FALSE(results.has_archive);
  ASSERT_EQ(2, results.archived_binary.size());
  ExpectBinary(kUnsignedExe, results.archived_binary.Get(0));
  ExpectBinary(kSignedExe, results.archived_binary.Get(1));
}

}  // namespace safe_browsing
