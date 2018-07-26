<?php

/**
 * Calling webservice endpoint : GET /webform_node_export
 *
 * @param $page
 *
 * @return mixed
 */
function os2forms_webform_sharing_get_webform_node_export($page = 0) {
  $getParams = http_build_query(
    array(
      'page' => $page,
      'options' => array(
        'orderby' => array(
          'changed' => 'DESC'
        )
      )
    )
  );
  $repoUrl = variable_get('os2forms_webform_sharing_repo_endpoint') . '/webform_node_export.json?'. $getParams;

  $options['headers']['Authorization'] = os2forms_webform_sharing_basic_auth();

  $result = drupal_http_request($repoUrl, $options);
  if ($result->code == 200) {
    return json_decode($result->data);
  }
}

/**
 * Calling webservice endpoint : GET /webform_node_export/uuid
 * @return object
 */
function os2forms_webform_sharing_get_webform_node_export_by_uuid($uuid) {
  $repoUrl = variable_get('os2forms_webform_sharing_repo_endpoint') . '/webform_node_export/' . $uuid . '.json';

  $options['headers']['Authorization'] = os2forms_webform_sharing_basic_auth();

  $result = drupal_http_request($repoUrl, $options);
  if ($result->code == 200) {
    $result = json_decode($result->data);
    if (!empty($result) && is_array($result)) {
      return array_pop($result);
    }
  }
}

/**
 * Calling webservice endpoint : POST /webform_node_export
 * @return object
 */
function os2forms_webform_sharing_post_webform_node_export($data) {
  $repoUrl = variable_get('os2forms_webform_sharing_repo_endpoint') . '/webform_node_export.json';

  $options['headers']['Authorization'] = os2forms_webform_sharing_basic_auth();
  $options['headers']['Content-Type'] = 'application/json';
  $options['method'] = 'POST';
  $options['data'] = json_encode(array(
    'node_export_data' => $data
  ));

  $result = drupal_http_request($repoUrl, $options);
  if ($result->code == 200) {
    return json_decode($result->data);
  }
}

/**
 * Calling webservice endpoint : GET /user
 *
 * @param $page
 *
 * @return object
 */
function os2forms_webform_sharing_get_user($page) {
  $getParams = http_build_query(
    array(
      'page' => $page,
      'options' => array(
        'orderby' => array(
          'created' => 'DESC'
        )
      )
    )
  );

  $repoUrl = variable_get('os2forms_webform_sharing_repo_endpoint') . '/user.json?' . $getParams;

  $options['headers']['Authorization'] = os2forms_webform_sharing_basic_auth();
  $result = drupal_http_request($repoUrl, $options);
  if ($result->code == 200) {
    return json_decode($result->data);
  }
}

/**
 * Generates basic authorization string ready to be used in request headers.
 * @return string
 */
function os2forms_webform_sharing_basic_auth() {
  $user = variable_get('os2forms_webform_sharing_repo_user');
  $password = variable_get('os2forms_webform_sharing_repo_password');

  return 'Basic ' . base64_encode($user . ':' . $password);
}