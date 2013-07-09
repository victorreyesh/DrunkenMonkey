<?php

/**
 * @file
 * Gradebook callback pages associated with displaying or editing a grade in the gradebook
 */

/**
 * Menu callback provides either an assignment summary (if user is a student)
 * or an edit form (if user is a teacher).
 *
 * All users with 'access gradebook' permissions can view this page.
 *
 * @param $gradebook
 *   The $gradebook object.
 * @param $uid
 *   Path argument for student user id whose grade we want to edit/view.
 * @param $nid
 *   Path argument for the node id for the assignment we want to edit/view.
 *
 * @return
 *   The grade edit or summary form.
 */
function gradebook_grade_page($gradebook, $uid, $nid) {

  $account = user_load($uid);

  if ($account !== FALSE && gradebookapi_is_student($gradebook, $account)) {
    $types = gradebookapi_get_assignment_types();
    $node = node_load($nid);
    if (in_array($node->type, $types)) {
      $grade = gradebookapi_get_grade($uid, $nid);
      $grades[$uid] = (array)$grade;
      $build['gradebook_grade_form'] = drupal_get_form('gradebook_grade_form', $gradebook, $grades);
      return $build;
    }
  }
}

/**
 * Menu callback to enter grades for all students by assignment.
 *
 *   This form is only accessible to teachers.
 *
 * @param $gradebook
 *   The $gradebook object.
 * @param $nid
 *   Path argument for the node id for the assignment we want to edit/view.
 *
 * @return
 *   The grade edit or summary form.
 */
function gradebook_grade_assignment_page($gradebook, $nid) {
  $types = gradebookapi_get_assignment_types();
  $node = node_load($nid);
  if (in_array($node->type, $types)) {
    $grades = array();
    $students = array();
    $students = gradebookapi_get_students($gradebook);
    foreach ($students['uid'] as $uid) {
      $grade = gradebookapi_get_grade($uid, $nid);
      $grades[$uid] = (array)$grade;
    }
    $build['gradebook_grade_form'] = drupal_get_form('gradebook_grade_form', $gradebook, $grades);
    return $build;
  }
  drupal_not_found();
}

/**
 * Menu callback to edit or provide a summary of student grades.
 *
 * Access is limited to teacher through the menu system.
 *
 * @param $gradebook
 *   The gradebook object.
 * @param $uid
 *   The user id.
 *
 * @return
 *   The grade edit or summary form.
 */
function gradebook_grade_student_page($gradebook, $uid) {
  // Get sort order for query.
  $default_sort = variable_get('gradebook_page_sort-' . $gradebook->tid, 'asc');
  $sort = (isset($_GET['sort']) && ($_GET['sort'] == 'asc')) ? 'asc' : 'desc';
  $default_order = variable_get('gradebook_page_order-' . $gradebook->tid, 'category');
  $order = isset($_GET['order']) ? $_GET['order'] : $default_order;

  $account = user_load($uid);
  if (gradebookapi_is_student($gradebook, $account)) {
    // get assignments
    $tids = array();
    $assignments = gradebook_get_sorted_assignments($gradebook, $tids, TRUE, $order, TRUE, $sort);

    // get grades for assignments
    $grades = array();
    foreach ($assignments as $assignment) {
      $grades[$assignment->nid] = (array)gradebookapi_get_grade($account->uid, $assignment->nid);
    }
    $build['gradebook_grade_form'] = drupal_get_form('gradebook_grade_form', $gradebook, $grades);
    return $build;
  }
  drupal_not_found();
}

/**
 * Form builder to edit a student grade.
 *
 * @param $gradebook
 *   The gradebook object.
 * @param $grades
 *   An array of grade objects.
 *
 * @see gradebook_grade_form_validate()
 * @see gradebook_grade_form_cancel()
 * @see gradebook_grade_form_submit()
 * @ingroup forms
 */
function gradebook_grade_form(&$form, &$form_state) {

global $user;
  $gradebook = $form_state['build_info']['args'][0];
  $grades =  $form_state['build_info']['args'][1];
  $teacher = gradebookapi_is_teacher($gradebook);
  $students = gradebookapi_get_students($gradebook);
  $suid = array_flip($students['uid']);
  // This is needed because keys of $students is lost in module_invoke_all call.

  $form['grade'] = array(
    '#tree'   => TRUE, // Don't forget to set #tree!
  );

//  @TODO: Can these be removed - can now access through form state, don't double include!
//  $form['#grades'] = $grades;  // Make grades available for hook_form_alter
//  $form['#gradebook'] = $gradebook; // Make gradebook available for hook_form_alter

  foreach ($grades as $id => $grade) {
    $uid = $grade['uid'];
    $display_name = $students['name'][$suid[$uid]];
    // This is needed because module_invoke_all loses the keys.

    // TODO: remove this - can always do this within hook_form_alter if its needed.
    // Make the node object available for hook_form_alter.
    $node = node_load($grade['nid']);

    $possible = isset($node->gradebookapi['possible']) ? $node->gradebookapi['possible'] : NULL;

    if (isset($form_state['values']['grade'][$id])) {
      $percentage =  $form_state['values']['grade'][$id]['percentage'];
      $oldearned =  $form_state['values']['grade'][$id]['earned'];
      $oldexempt = $form_state['values']['grade'][$id]['exempt'];
      $oldnote = $form_state['values']['grade'][$id]['note'];
    }
    else {
      $percentage = (!isset($grade['earned'])) ? NULL : (isset($possible) && $possible != 0) ? 100.0*$grade['earned']/$possible : 0;
      $oldearned = isset($grade['earned']) ? $grade['earned'] : NULL;
      $oldexempt = isset($grade['exempt']) ? $grade['exempt'] : 0;
      $oldnote = isset($grade['note']) ? $grade['note'] : '';
    }
 //   $form['grade'][$id]['#node'] = $node;  // TODO - can this be removed?
    $form['grade'][$id]['uid'] = array('#type' => 'value', '#value' => $uid);  // must be '#type' => 'value'
    $form['grade'][$id]['nid'] = array('#type' => 'value', '#value' => $node->nid); // must be '#type' => 'value'
    $form['grade'][$id]['user'] = array('#markup' => $display_name);
    $form['grade'][$id]['node'] = array('#markup' => $node->title);
    $form['grade'][$id]['#possible'] = $possible;
    $form['grade'][$id]['#oldpercentage'] = $percentage;
    $form['grade'][$id]['#oldearned'] = $oldearned;
    $form['grade'][$id]['timestamp'] = array('#type' => 'value', '#value' => $grade['timestamp']);
    if ($teacher) {
      // Displays an edit form.
      $form['grade'][$id]['earned'] = array(
        '#type' => 'textfield',
        '#size' => 4,
        '#default_value' => $oldearned,
        '#field_suffix' => '/' . $possible,
        '#prefix' => '<div id = "earned-' . $id . '-wrapper">',
        '#suffix' => '</div>',
        '#ajax' => array(
          'event'   => 'change',
          'callback'    => 'gradebook_earned_percentage_callback',
          'wrapper' => 'percentage-' . $id . '-wrapper',
        ),
      );

      $form['grade'][$id]['percentage'] = array(
        '#type' => 'textfield',
        '#size' => 4,
        '#default_value' => $percentage,
        '#field_suffix' => '%',
        '#prefix' => '<div id = "percentage-' . $id . '-wrapper">',
        '#suffix' => '</div>',
        '#ajax' => array(
          'event'   => 'change',
          'callback'    => 'gradebook_earned_percentage_callback',
          'wrapper' => 'earned-' . $id . '-wrapper',
        ),
      );

      $form['grade'][$id]['exempt'] = array('#type' => 'checkbox', '#default_value' => $oldexempt);
      $form['grade'][$id]['note'] = array('#type' => 'textarea', '#cols' => 40, '#rows' => 6, '#default_value' => $oldnote);
    }
    else {
      // Displays grades without an option for editing.
      if ($oldearned != '') {
        $grade_mark = gradebookapi_load_marks($gradebook->tid, $node->nid, $percentage, FALSE);
        if (!$grade_mark) {
          $grade_mark = $oldearned;
          $suffix =  '/' . $possible;
        }
      $form['grade'][$id]['earned'] = array('#value' => $grade_mark, '#suffix' => $suffix);
      $form['grade'][$id]['percentage'] = array('#value' => $percentage, '#suffix' => '%');
      }
      $form['grade'][$id]['exempt'] = array('#type' => 'checkbox', '#value' => $oldexempt, '#disabled' => TRUE);
      $form['grade'][$id]['note'] = array('#value' => $oldnote);
    }
  }
  if ($teacher && count($grades) > 1) {
    $form['default'] = array(
      '#tree'   => TRUE, // Don't forget to set #tree!
    );
    $form['default']['user'] = array('#markup' => t('Default'));
    $form['default']['node'] = array('#markup' => $node->title);
    $form['default']['#possible'] = $possible;

    $oldearned = isset($form_state['values']['default']['earned']) ? $form_state['values']['default']['earned'] : NULL;
    $percentage = isset($form_state['values']['default']['percentage']) ? $form_state['values']['default']['percentage'] : NULL;

    $form['default']['#oldpercentage'] = $percentage;
    $form['default']['#oldearned'] = $oldearned;
    $form['default']['earned'] = array(
      '#type' => 'textfield',
      '#size' => 4,
      '#default_value' => $oldearned,
      '#field_suffix' => '/' . $possible,
      '#prefix' => '<div id = "earned-default-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'event' => 'change',
        'callback' => 'gradebook_earned_percentage_callback',
        'wrapper' => 'percentage-default-wrapper',
      ),
      '#tree' => TRUE,
    );

    $form['default']['percentage'] = array(
      '#type' => 'textfield',
      '#size' => 4,
      '#default_value' => $percentage,
      '#field_suffix' => '%',
      '#prefix' => '<div id = "percentage-default-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'event' => 'change',
        'callback' => 'gradebook_earned_percentage_callback',
        'wrapper' => 'earned-default-wrapper',
        ),
      '#tree' => TRUE,
    );
    $form['default']['exempt'] = array('#type' => 'checkbox', '#default_value' => 0);
    $form['default']['note'] = array('#type' => 'textarea', '#cols' => 40, '#rows' => 6, '#default_value' => '');
  }

  $modules = module_implements('gradebook_form_add_elements');
  foreach ($modules as $module) {
    $func = $module . '_gradebook_form_add_elements';
    $func($form, $form_state, $grades, $gradebook);
  }
  $form['return_url'] = array('#type' => 'value', '#value' => 'gradebook/' . $gradebook->tid);
  $form['teacher'] = array('#type' => 'value', '#value' => $teacher);
  $form['actions'] = array('#type' => 'actions');
  if ($teacher) {
    $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save All'));
    if (isset($form['default'])) {
      $form['default_submit'] = array('#type' => 'submit', '#value' => t('Apply Default'), '#validate' => array('gradebook_grade_form_default_validate'));
    }
  }
  $form['actions']['cancel'] = array('#type' => 'submit', '#value' =>  t('Return to Gradebook'), '#submit' => array('gradebook_grade_form_cancel'));
  return $form;
}

function gradebook_grade_form_validate($form, &$form_state) {
  // No validation is needed for floating point grades, but saving this function for future development.
}

function gradebook_grade_form_cancel($form, &$form_state) {
  // Clear the storage bins now that files have been saved - needed for redirect.
  $form_state['redirect'] = $form_state['values']['return_url'];
  unset($form_state['values']['return_url']);
}

function gradebook_grade_form_submit(&$form, &$form_state) {
  $keys = array_keys($form_state['values']['grade']);
  $count = 0;
  foreach ($keys as $key) {
    $grade = (object)$form_state['values']['grade'][$key];
    if ($form_state['values']['teacher']) {
      $node = node_load($form_state['values']['grade'][$key]['nid']);
      $grade->due_date = $node->gradebookapi['due_date'];
      $grade->publish_date = $node->gradebookapi['publish_date'];
      if (isset($form['grade'][$key]['percentage']['#disabled']) || isset($form['grade'][$key]['earned']['#disabled'])) {
        $grade->earned = $form['grade'][$key]['#oldearned'];
        $grade->percentage = $form['grade'][$key]['#oldpercentage'];
      }
      else {
        $percentage = $form_state['values']['grade'][$key]['percentage'];
        $earned = $form_state['values']['grade'][$key]['earned'];
        // Use whichever changed... if both changed, use points.
        if ($earned != $form['grade'][$key]['#oldearned']) {
          $grade->earned = $earned;
        }
        elseif ($percentage != $form['grade'][$key]['#oldpercentage']) {
          $grade->earned = $percentage*$form['grade'][$key]['#possible']/100.0;
        }
      }
      $grade->exempt = isset($form['grade'][$key]['exempt']['#disabled']) ? $form['grade'][$key]['exempt']['#value'] : $form_state['values']['grade'][$key]['exempt'];
      gradebookapi_set_grade($grade);
    }
    else {
      if ($grade->timestamp == 0) {
        // Grade doesn't exist in gradebookapi_grade, save a null grade.
        gradebookapi_set_grade($grade);
      }
      else { // Grade already exists, but give other modules a chance to do something here.
        module_invoke_all('gradebookapi_grade', 'set', $grade);
      }
    }
    ++$count;
  }
  if ($count > 0) {
    drupal_set_message(t('Saved %num %type.', array('%num' => $count, '%type' => t('grades'))));
  }
  else {
    drupal_set_message(t('No changes were made to grades'));
  }
  $form_state['redirect'] = $form_state['values']['return_url'];
  unset($form_state['values']['return_url']);
}

function gradebook_grade_form_default_validate(&$form, &$form_state) {
  // Apply default grade to all grade form fields.

  // Determined which changed and use it to update the other...
  $earned = NULL;
  $percentage = NULL;
  if (isset($form_state['values']['default']['earned'])) {
    if (!isset($form['default']['#oldearned']) ||
      ($form['default']['#oldearned'] != $form_state['values']['default']['earned'])) {
        // Earned has been changed. Adjust percentage accordingly
      $earned = $form_state['values']['default']['earned'];
      $percentage = isset($form['default']['#possible']) ? 100.0*$earned/$form['default']['#possible'] : NULL;
      // Update the default percentage value on the form.
      $form['default']['percentage']['#value'] = $percentage;
    }
  }
  if (!isset($earned) && (isset($form_state['values']['default']['percentage']))) {
    // Earned didn't change, check percentage.
    if (!isset($form['default']['#oldpercentage']) ||
      ($form['default']['#oldpercentage'] != $form_state['values']['default']['percentage'])) {
      // Percentage has been changed. Adjust earned accordingly.
      $percentage = $form_state['values']['default']['percentage'];
      $earned = isset($form['default']['#possible']) ? 0.01*$form['default']['#possible']*$percentage : NULL;
      // Update the default earned value on the form.
      $form['default']['earned']['#value'] = $earned;
    }
  }
  $exempt = $form_state['values']['default']['exempt'];
  $note = $form_state['values']['default']['note'];

  // Update the form with the default values.
  $keys = array_keys($form_state['values']['grade']);
  foreach ($keys as $key) {
    $form['grade'][$key]['earned']['#value'] = $earned;
    $form['grade'][$key]['percentage']['#value'] = $percentage;
    $form['grade'][$key]['exempt']['#value'] = $exempt;   // TODO: Why doesn't this work, when all the rest do?
    $form['grade'][$key]['note']['#value'] = $note;
  }
  drupal_set_message(t('Default grade applied. Grades have not yet been saved.'));

  // Not really an error, but we need to stop form processing to allow teacher
  // to reflect on the change, and make adjustments if needed.
  form_set_error('default_submit');
}

/**
 * Rebuild the gradebook form page when the earned or percentage field is updated.
 * If the earned field is changed, the perctange field is rendered and vice versa.
 * Only the modified row is rendered again.
 *
 * @param $gradebook
 *   The gradebook object
 * @param $id
 *   The array index to be updated
 * @param $field
 *   The field (either earned or percentage) to be updated.
 */
function gradebook_earned_percentage_callback($form, $form_state) {
  $id = $form_state['triggering_element']['#ajax']['wrapper'];
  $trigger_value = $form_state['triggering_element']['#value'];
  $parts = explode("-", $id);
  $field = $parts[0];
  $key = $parts[1];
  if ($key == 'default') {
    $parent = $form['default'];
  }
  else {
    $parent = $form['grade'][$key];
  }
  $element = $parent[$field];
  // Only change the field value if the trigger value and points possible have been defined.
  if ((isset($parent['#possible']) && $parent['#possible'] != 0) && isset($trigger_value)) {
    $possible = $parent['#possible'];

    switch ($field) {
      case 'earned':
        $element['#value'] = 0.01*$trigger_value*$possible;
      break;
      case 'percentage':
        $element['#value'] = 100.0*$trigger_value/$possible;
      break;
    }
  }
  return $element;
}

/**
 * Returns HTML to theme the grade form.
 *
 * @return
 *   The HTML string ready to display.
 ;*
 * @ingroup themeable
 */
function theme_gradebook_grade_form($variables) {
  global $user;

  $form = $variables['form'];
  $header = array(
    'name' => array('data' => t('Name')),
    'title' => array('data' => t('Assignment Title')),
    'grade' => array('data' => t('Grade')),
    'exempt' => array('data' => t('Exempt')),
    'note' => array('data' => t('Note')),
  );

  $ids = element_children($form['grade']);

  foreach ($ids as $id) {
    $row = array();
    $row[] = array('data' => drupal_render($form['grade'][$id]['user']), 'class' => 'name', 'id' => 'name-' . $form['grade'][$id]['#id']);
    // Allow modules to add information to the title block.
    $data = drupal_render($form['grade'][$id]['node']);
    $module_output = _gradebook_module_additions('title', $form, $id, $form['teacher']['#value'], '<br />');
    if ($module_output != '') {
      $data .= '<br />' . $module_output;
    }
    $row[] = array('data' => $data , 'class' => 'title',  'id' => 'title-' . $form['grade'][$id]['#id']);
    if (isset($form['grade'][$id]['earned'])) {
      $row[] = array(
        'data' => drupal_render($form['grade'][$id]['earned']) . '<br />' . drupal_render($form['grade'][$id]['percentage']),
        'class' => 'earned-possible',
        'id' => 'earned-possible' . $form['grade'][$id]['#id'],
      );
    }
    else {
      $row[] = array('data' => '&nbsp', 'class' => 'earned-possible');
    }
    $row[] = array('data' => drupal_render($form['grade'][$id]['exempt']), 'class' => 'exempt-box');
    $output = '<div class = "note-region">' . drupal_render($form['grade'][$id]['note']) . '</div>';
    // Give modules a chance to add output below the note block.
    // Can't use module_invoke, module_invoke_all, or call_user_func, because these do not pass arguments by reference!
    $output .= _gradebook_module_additions('note', $form, $id, $form['teacher']['#value']);
    $row[] = array('data' => $output, 'class' => 'note');
    $rows[$id] = $row;  // was array('data' => $row);
  }

  $drows = array();

  if (isset($form['default'])) {
    $row = array();
    $row[] = array('data' => drupal_render($form['default']['user']), 'class' => 'name');
    $data = drupal_render($form['default']['node']);
    $row[] = array('data' => $data , 'class' => 'title');
    if (isset($form['default']['earned'])) {
      $row[] = array('data' => drupal_render($form['default']['earned']) . '<br />' . drupal_render($form['default']['percentage']), 'class' => 'earned-possible');
    }
    else {
      $row[] = array('data' => '&nbsp', 'class' => 'earned-possible');
    }
    $row[] = array('data' => drupal_render($form['default']['exempt']), 'class' => 'exempt-box');
    $output = '<div class = "note-region">' . drupal_render($form['default']['note']) . '</div>';
    // Give modules a chance to add output below the note block.
    // Can't use module_invoke, module_invoke_all, or call_user_func, because these do not pass arguments by reference!
    $output .= _gradebook_module_additions('note', $form, 'default', $form['teacher']['#value']);
    $row[] = array('data' => $output, 'class' => 'note');
    $drows[] = $row;

  }
  // Allow modules to adjust table,
  $modules = module_implements('gradebook_adjust_grade_form_table');
  $table = array('header' => $header, 'drows' => $drows, 'rows' => $rows,  'attributes' => array('class' => array('gradebook')));

  foreach ($modules as $module) {
    $func = $module . '_gradebook_adjust_grade_form_table';
    $table = $func($form, $table);
  }

  if (count($drows) > 0) {
    $output = '<div><h3>' . t('Default Grade') . '</h3>';
    $output .= theme('table',  array('header' => $table['header'], 'rows' => $table['drows']));
    $output .= '<div>' . drupal_render($form['default_submit']) . '</div>';
    $output .= '</div>';
  }
  else {
    $output = '';
  }

  $output .= theme('table', array('header' => $table['header'], 'rows' => $table['rows'], 'attributes' => array('class' => array('gradebook'))));
  $output .= theme('pager');
  $output .= '<div>' . _gradebook_module_additions('bottom', $form, -1, $form['teacher']['#value']) . '</div>';
  $output .= '<div>' . drupal_render($form['submit']) . drupal_render($form['cancel']) . '</div>';
  $output .= drupal_render_children($form);

  return $output;
}


/**
 * Gathers HTML to add to the gradebook grade form from submodules
 * hook_gradebook_form_add_content.
 *
 * @param $position
 *   The position on the form to insert the content.
 * @param $form
 *   The form array, passed by reference.
 * @param $teacher
 *   TRUE if the current user is a teacher.
 * @param $separator
 *   HTML element to separate the content from each submodule.
 *
 * @return
 *   HTML to add to the grade form.
 */
function _gradebook_module_additions($position, &$form, $id, $teacher, $separator = '') {
  $output = '';
  $modules = module_implements('gradebook_form_add_content');
  foreach ($modules as $module) {
    $func = $module . '_gradebook_form_add_content';
    $addition = $func($position, $form, $id, $teacher);  // need to pass by reference!
    if ($addition != '') {
      $output .= $addition;
      $output .= $separator;
    }
  }
  return $output;
}

/**
 * Rebuilds the grades and returns the user to the gradebook page.
 *
 * @param $gradebook
 *   The gradebook object.
 *
 */
function gradebook_rebuild_page($gradebook) {
  gradebookapi_rebuild_grades($gradebook->tid);
  drupal_goto('gradebook/' . $gradebook->tid);
}
