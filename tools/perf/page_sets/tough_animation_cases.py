# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class ToughAnimationCasesPage(page_module.Page):

  def __init__(self, url, page_set, need_measurement_ready):
    super(ToughAnimationCasesPage, self).__init__(url=url, page_set=page_set)
    self.archive_data_file = 'data/tough_animation_cases.json'
    self._need_measurement_ready = need_measurement_ready

  def RunNavigateSteps(self, action_runner):
    action_runner.NavigateToPage(self)
    if self._need_measurement_ready:
      action_runner.WaitForJavaScriptCondition('window.measurementReady')

  def RunPageInteractions(self, action_runner):
    action_runner.Wait(10)

class ToughAnimationCasesPageSet(page_set_module.PageSet):

  """
  Description: A collection of animation performance tests
  """

  def __init__(self):
    super(ToughAnimationCasesPageSet, self).__init__(
      archive_data_file='data/tough_animation_cases.json',
      bucket=page_set_module.PARTNER_BUCKET)

    urls_list_one = [
      # Why: Tests the balls animation implemented with SVG animations.
      'file://tough_animation_cases/balls_svg_animations.html',
      # Why: Tests the balls animation implemented with Javascript and canvas.
      'file://tough_animation_cases/balls_javascript_canvas.html',
      # Why: Tests the balls animation implemented with Javascript and CSS.
      'file://tough_animation_cases/balls_javascript_css.html',
      # Why: Tests the balls animation implemented with CSS keyframe animations.
      'file://tough_animation_cases/balls_css_keyframe_animations.html',
      # Why: Tests the balls animation implemented with transforms and CSS
      # keyframe animations to be run on the compositor thread.
      # pylint: disable=C0301
      'file://tough_animation_cases/balls_css_keyframe_animations_composited_transform.html',
      # Why: Tests the balls animation implemented with CSS transitions on 2
      # properties.
      'file://tough_animation_cases/balls_css_transition_2_properties.html',
      # Why: Tests the balls animation implemented with CSS transitions on 40
      # properties.
      'file://tough_animation_cases/balls_css_transition_40_properties.html',
      # Why: Tests the balls animation implemented with CSS transitions on all
      # animatable properties.
      'file://tough_animation_cases/balls_css_transition_all_properties.html',
      # pylint: disable=C0301
      'file://tough_animation_cases/overlay_background_color_css_transitions.html',

      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_simultaneous_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_simultaneous_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_simultaneous_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_simultaneous_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_updating_inline_style.html?N=0316',

      # Why: Tests many CSS Animations all starting at the same time with 500
      # keyframes each.
      'file://tough_animation_cases/css_animations_many_keyframes.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_simultaneous_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_simultaneous_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_simultaneous_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_simultaneous_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_chaining_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_chaining_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_chaining_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_chaining_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # inserting new elements.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_triggering_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time with
      # staggered animation offsets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_infinite_iterations.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # inserting style sheets.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_triggering_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # updating class.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_triggering_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # updating inline style.
      # pylint: disable=C0301
      'file://tough_animation_cases/css_animations_staggered_triggering_by_updating_inline_style.html?N=0316',

      # Why: Tests many Web Animations all starting at the same time with 500
      # keyframes each.
      'file://tough_animation_cases/web_animations_many_keyframes.html?N=0316',
      # Why: Tests many paused Web Animations having their currentTimes updated
      # in every requestAnimationFrame.
      # pylint: disable=C0301
      'file://tough_animation_cases/web_animations_set_current_time_in_raf.html?N=0316',
      # Why: Tests many Web Animations all starting at the same time.
      'file://tough_animation_cases/web_animations_simultaneous.html?N=0316',
      # Why: Tests many Web Animations all starting at different times then
      # chained together using events.
      # pylint: disable=C0301
      'file://tough_animation_cases/web_animations_staggered_chaining.html?N=0316',
      # Why: Tests many Web Animations all starting at different times with
      # infinite iterations.
      # pylint: disable=C0301
      'file://tough_animation_cases/web_animations_staggered_infinite_iterations.html?N=0316',
      # Why: Tests many Web Animations all starting at different times.
      # pylint: disable=C0301
      'file://tough_animation_cases/web_animations_staggered_triggering.html?N=0316',
    ]

    for url in urls_list_one:
      self.AddUserStory(ToughAnimationCasesPage(url, self,
                                           need_measurement_ready=True))

    urls_list_two = [
      # Why: Tests various keyframed animations.
      'file://tough_animation_cases/keyframed_animations.html',
      # Why: Tests various transitions.
      'file://tough_animation_cases/transform_transitions.html',
      # Why: JS execution blocks CSS transition unless initial transform is set.
      'file://tough_animation_cases/transform_transition_js_block.html'

      # Disabled: crbug.com/350692
      # Why: Login page is slow because of ineffecient transform operations.
      # 'http://ie.microsoft.com/testdrive/performance/robohornetpro/',
    ]

    for url in urls_list_two:
      self.AddUserStory(ToughAnimationCasesPage(url, self,
                                           need_measurement_ready=False))
