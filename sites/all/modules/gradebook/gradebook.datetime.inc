<?php
/**
 * @file
 * Helper functions for gradebook dates and times
 */

/**
 *  These date functions are copied and adapted from the announcements module
 *  TODO: document these functions
 */

function _gradebookapi_get_timezone() {
  global $user;
  if (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
    // User has his/her own timezone, use that.
    return $user->timezone;
  }
  return variable_get('date_default_timezone', 0);
}

function _gradebookapi_drupaldate2unixtime($drupal_date) {
  // Compute the time in UTC (GMT).
  $tz_seconds = _gradebookapi_get_timezone();
  return gmmktime(0, 0, 0, (int)$drupal_date["month"], (int)$drupal_date["day"], (int)$drupal_date["year"]) - $tz_seconds;
}

function _gradebookapi_unixtime2drupaldate($unixtime) {
  $unixtime -= _gradebookapi_get_timezone();
  return array('day' => gmdate('j', $unixtime),
    'month' => gmdate('n', $unixtime),
    'year' => gmdate('Y', $unixtime));
}

function _gradebookapi_datepopup2unixtime($popup_date) {
  // TODO: Consider timezone for popup? If so, what does the form element need to be to make it work?
  return strtotime($popup_date);
}

function _gradebookapi_unixtime2datepopup($unixtime) {
// TODO: Consider timezone for popup? If so, what does the form element need to be to make it work?
  return date("Y-m-d H:i", $unixtime);
}
