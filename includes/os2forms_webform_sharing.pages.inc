<?php

/**
 * Menu callback for generating list of shared/remote webforms.
 * @return string
 */
function os2forms_webform_sharing_list_shared() {
  $per_page = 20;

  $html = '';

  $header = array(
    array(
      'data' => t('Remote form name'),
      'field' => 'f.title',
    ),
    array(
      'data' => t('Remote form author'),
      'field' => 'f.ruid',
    ),
    array(
      'data' => t('Readonly'),
      'field' => 'f.readonly',
    ),
    array(
      'data' => t('Remote form updated'),
      'field' => 'f.changed',
      'sort' => 'desc'
    ),
    array(
      'data' => t('Local form updated'),
      'field' => 'n.changed',
      'sort' => 'desc'
    ),
    array(
      'data' => t('Status'),
      'field' => 'f.lnid',
    ),
    array(
      'data' => t('Actions'),
    ),
  );
  $query = db_select('os2forms_webform_sharing_repo_webforms', 'f');
  $query->join('os2forms_webform_sharing_repo_users', 'u', 'f.ruid = u.uid');
  $query->leftJoin('node', 'n', 'n.nid = f.lnid');
  $query = $query->extend('TableSort');
  $query->fields('f');
  $query->fields('u', array('name'));
  $query->addField('n', 'changed', 'local_node_changed');
  $result = $query->orderByHeader($header)->execute();
  $rows = array();
  foreach ($result as $row) {
    $data = array();
    $data[] = $row->title;
    $data[] = $row->name;
    $data[] = ($row->readonly) ? t('Yes') : t('No');
    $data[] = format_date($row->changed);
    $data[] = ($row->local_node_changed) ? format_date($row->local_node_changed) : '';
    if ($row->lnid) {
      $actions = '';
      if ($row->local_node_changed < $row->changed) {
        $data[] = t('Cloned | Update available');
        $actions .= l(t('Update'), 'admin/config/content/os2forms-repository/' . $row->uuid . '/update', array('query' => array('destination' => 'admin/config/content/os2forms-repository/shared')));
        $actions .= ' | ';
      }
      elseif ($row->local_node_changed > $row->changed) {
        $data[] = t('Cloned | Locally changed');
        $actions .= l(t('Reset'), 'admin/config/content/os2forms-repository/' . $row->uuid . '/reset', array('query' => array('destination' => 'admin/config/content/os2forms-repository/shared')));
        $actions .= ' | ';
      }
      else {
        $data[] = t('Cloned');
      }
      $actions .= l(t('Open local webform'), 'node/' . $row->lnid, array('attributes' => array('target' => '_blank')));
      $data[] = $actions;
    }
    else {
      $data[] = t('Available');
      if (user_access('use PHP to import nodes')) {
        $data[] = l(t('Clone'), 'admin/config/content/os2forms-repository/' . $row->uuid . '/clone', array('query' => array('destination' => 'admin/config/content/os2forms-repository/shared')));
      }
      else {
        $data[] = '';
      }
    }

    $rows[] = array('data' => $data, 'class' => array('row_phrase'), 'data-id' => array());
  }

  // Initialize the pager
  $current_page = pager_default_initialize(count($rows), $per_page);
  // Split your list into page sized chunks
  $chunks = array_chunk($rows, $per_page, TRUE);

  // Build the table for the nice output.
  $build['tablesort_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => !empty($chunks) ? $chunks[$current_page] : array(),
  );
  $html .= drupal_render($build);

  // Show the pager
  $html .= theme('pager', array('quantity', count($rows)));

  return $html;
}

/**
 * Webform clone confirm form.
 *
 * @param $form
 * @param $form_state
 * @param null $webform_uuid
 *
 * @return mixed
 */
function os2forms_webform_sharing_clone_form($form, &$form_state, $webform_uuid = NULL) {
  $webform = os2forms_webform_sharing_repo_load_webform($webform_uuid);

  $form['uuid'] = array('#type' => 'value', '#value' => $webform_uuid);

  return confirm_form($form,
    t('%title form will be cloned to your local environment?', array('%title' => $webform->title)),
    'admin/config/content/os2forms-repository',
    t('Confirm operation.')
  );
}

/**
 * Webform clone/update/reset confirm form submit handler.
 *
 * @param $form
 * @param $form_state
 */
function os2forms_webform_sharing_clone_form_submit($form, &$form_state) {
  if ($form_state['values']['uuid']) {
    $batch = os2forms_webform_sharing_batch_clone(array($form_state['values']['uuid']));
    batch_set($batch);
  }
}

/**
 * Menu callback for generating list of local webforms.
 * @return string
 */
function os2forms_webform_sharing_list_local() {
  $per_page = 20;

  $html = '';

  $header = array(
    array(
      'data' => t('Local form name'),
      'field' => 'n.title',
    ),
    array(
      'data' => t('Local form author'),
      'field' => 'n.uid',
    ),
    array(
      'data' => t('Readonly'),
      'field' => 'f.readonly',
    ),
    array(
      'data' => t('Last updated'),
      'field' => 'n.changed',
      'sort' => 'desc'
    ),
    array(
      'data' => t('Status'),
      'field' => 'f.lnid',
    ),
    array(
      'data' => t('Actions'),
    ),
  );
  $query = db_select('node', 'n');
  $query->join('users', 'u', 'n.uid = u.uid');
  $query->leftJoin('os2forms_webform_sharing_repo_webforms', 'f', 'f.uuid = n.uuid');
  $query = $query->extend('TableSort');
  $query->fields('n');
  $query->fields('u', array('name'));
  $query->fields('f', array('lnid', 'readonly'));
  $query->addField('f', 'uuid', 'remote_uuid');
  $query->condition('n.status', 1);
  $query->condition('n.type', 'webform');
  $result = $query->orderByHeader($header)->execute();
  $rows = array();
  foreach ($result as $row) {
    $data = array();
    $data[] = l($row->title, 'node/' . $row->nid);
    $data[] = l($row->name, 'user/' . $row->uid);
    $data[] = ($row->readonly) ? t('Yes') : t('No');
    $data[] = format_date($row->changed);

    if ($row->remote_uuid) {
      $data[] = t('Linked');
      $data[] = l(t('Unlink'), 'admin/config/content/os2forms-repository/' . $row->uuid . '/unlink', array('query' => array('destination' => 'admin/config/content/os2forms-repository/local')));
    }
    else {
      $data[] = t('Unlinked');
      if (user_access('export nodes')) {
        $data[] = l(t('Commit'), 'admin/config/content/os2forms-repository/' . $row->uuid . '/commit', array('query' => array('destination' => 'admin/config/content/os2forms-repository/local')));
      }
      else {
        $data[] = '';
      }
    }

    $rows[] = array('data' => $data, 'class' => array('row_phrase'), 'data-id' => array());
  }

  // Initialize the pager
  $current_page = pager_default_initialize(count($rows), $per_page);
  // Split your list into page sized chunks
  $chunks = array_chunk($rows, $per_page, TRUE);

  // Build the table for the nice output.
  $build['tablesort_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => !empty($chunks) ? $chunks[$current_page] : array(),
  );
  $html .= drupal_render($build);

  // Show the pager
  $html .= theme('pager', array('quantity', count($rows)));

  return $html;
}

/**
 * Webform commit confirm form.
 *
 * @param $form
 * @param $form_state
 * @param null $webform_uuid
 *
 * @return mixed
 */
function os2forms_webform_sharing_commit_form($form, &$form_state, $webform_uuid = NULL) {
  $webform = os2forms_webform_sharing_load_webform($webform_uuid);

  $form['uuid'] = array('#type' => 'value', '#value' => $webform_uuid);

  return confirm_form($form,
    t('%title form will be committed to remote server?', array('%title' => $webform->title)),
    'admin/config/content/os2forms-repository',
    t('Confirm operation.')
  );
}

/**
 * Webform commit confirm form submit handler.
 *
 * @param $form
 * @param $form_state
 */
function os2forms_webform_sharing_commit_form_submit($form, &$form_state) {
  if ($form_state['values']['uuid']) {
    $batch = os2forms_webform_sharing_batch_commit(array($form_state['values']['uuid']));
    batch_set($batch);
  }
}

/**
 * Webform unlink confirm form.
 *
 * @param $form
 * @param $form_state
 * @param null $webform_uuid
 *
 * @return mixed
 */
function os2forms_webform_sharing_unlink_form($form, &$form_state, $webform_uuid = NULL) {
  $webform = os2forms_webform_sharing_load_webform($webform_uuid);

  $form['uuid'] = array('#type' => 'value', '#value' => $webform_uuid);

  return confirm_form($form,
    t('%title form will be unlinked from the remote server?', array('%title' => $webform->title)),
    'admin/config/content/os2forms-repository',
    t('Unlinking form means that the form becomes detached from the remote version and will not receive any new form updates.')
  );
}

/**
 * Webform unlink confirm form submit handler.
 *
 * @param $form
 * @param $form_state
 */
function os2forms_webform_sharing_unlink_form_submit($form, &$form_state) {
  if ($form_state['values']['uuid']) {
    $batch = os2forms_webform_sharing_batch_unlink(array($form_state['values']['uuid']));
    batch_set($batch);
  }
}

/**
 * Webform update confirm form.
 *
 * @param $form
 * @param $form_state
 * @param null $webform_uuid
 *
 * @return mixed
 */
function os2forms_webform_sharing_update_form($form, &$form_state, $webform_uuid = NULL) {
  $webform = os2forms_webform_sharing_load_webform($webform_uuid);

  $form['uuid'] = array('#type' => 'value', '#value' => $webform_uuid);
  $form['#submit'][] = 'os2forms_webform_sharing_clone_form_submit';

  return confirm_form($form,
    t('%title form will be overwritten with the newest version from the remote server?', array('%title' => $webform->title)),
    'admin/config/content/os2forms-repository',
    t('Updating will revert any local changes that were possible done to the form.')
  );
}

/**
 * Webform reset confirm form.
 *
 * @param $form
 * @param $form_state
 * @param null $webform_uuid
 *
 * @return mixed
 */
function os2forms_webform_sharing_reset_form($form, &$form_state, $webform_uuid = NULL) {
  $webform = os2forms_webform_sharing_load_webform($webform_uuid);

  $form['uuid'] = array('#type' => 'value', '#value' => $webform_uuid);
  $form['#submit'][] = 'os2forms_webform_sharing_clone_form_submit';

  return confirm_form($form,
    t('%title form will be reset to the last version from the remote server?', array('%title' => $webform->title)),
    'admin/config/content/os2forms-repository',
    t('Resetting will revert any local changes that were done to the form.')
  );
}

/**
 * Page callback for settings form.
 *
 * @param $form
 * @param $form_state
 *
 * @return mixed
 */
function os2forms_webform_sharing_page_settings($form, &$form_state) {
  // Attempt the webservice request.
  if (os2forms_webform_sharing_get_webform_node_export()) {
    drupal_set_message(t('Sharing repository set up correctly'));
  }
  else {
    drupal_set_message(t('Could not connect to the repository. Check connection credentials'), 'error', FALSE);
  }

  // Do synch, if asked.
  if (isset($form_state['triggering_element']) && $form_state['triggering_element']['#id'] == 'edit-synch-now') {
    os2forms_webform_sharing_cron();
  }

  $form['os2forms_webform_sharing_repo_endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository Endpoint URL'),
    '#default_value' => variable_get('os2forms_webform_sharing_repo_endpoint'),
    '#description' => t('The service endpoint URL of the remote repository. Do no add trailing slash!'),
    '#required' => TRUE,
  );

  $form['os2forms_webform_sharing_repo_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository username'),
    '#default_value' => variable_get('os2forms_webform_sharing_repo_user'),
    '#description' => t('The service username.'),
    '#required' => TRUE,
  );

  $form['os2forms_webform_sharing_repo_password'] = array(
    '#type' => 'password',
    '#title' => t('Repository password'),
    '#description' => t('The service password. Leave blank if not changed'),
  );

  $form['os2forms_webform_sharing_repo_synch_now_fs'] = array(
    '#type' => 'fieldset',
    '#title' => t('Synch now'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['os2forms_webform_sharing_repo_synch_now_fs']['synch_now'] = array(
    '#type' => 'button',
    '#value' => t('Synch now'),
  );

  $form['os2forms_webform_sharing_repo_synch_now_fs'][] = array(
    '#markup' => '<div>' . t('Last synched: @datetime', array('@datetime' => format_date(variable_get('os2forms_webform_sharing_last_synch', 0)))) . '</div>'
  );

  $form = system_settings_form($form);
  array_unshift($form['#submit'], 'os2forms_webform_sharing_page_settings_submit');

  return $form;
}

/**
 * Additional os2forms_webform_sharing_page_settings form submit handler.
 * Unsetting password on form submit if password is empty.
 *
 * @param $form
 * @param $form_state
 */
function os2forms_webform_sharing_page_settings_submit($form, &$form_state) {
  if ($form_state['values']['os2forms_webform_sharing_repo_password'] == '') {
    unset($form_state['values']['os2forms_webform_sharing_repo_password']);
  }
}
