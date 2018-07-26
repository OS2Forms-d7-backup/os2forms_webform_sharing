<?php

/**
 * Inserts or updates remote user information.
 *
 * @param $user
 */
function os2forms_webform_sharing_repo_insert_update_user($user) {
  // Ignoring anonymous user.
  if ($user->uid) {
    $fields['uuid'] = $user->uuid;
    $fields['uid'] = $user->uid;
    $fields['name'] = $user->name;
    $fields['mail'] = $user->mail;

    $query = db_select('os2forms_webform_sharing_repo_users', 'u')
      ->fields('u')
      ->condition('uuid', $user->uuid)
      ->execute()
      ->fetchAll();

    if (!empty($query)) {
      db_update('os2forms_webform_sharing_repo_users')
        ->fields($fields)
        ->condition('uuid', $user->uuid)
        ->execute();
    }
    else {
      db_insert('os2forms_webform_sharing_repo_users')
        ->fields($fields)
        ->execute();
    }
  }
}

/**
 * Loads webform node by its UUID.
 *
 * @param $uuid
 *
 * @return mixed|null
 */
function os2forms_webform_sharing_load_webform($uuid) {
  $webform = NULL;

  $entities = entity_uuid_load('node', array($uuid));
  if (!empty($entities) && is_array($entities)) {
    $webform = reset($entities);
  }

  return $webform;
}

/**
 * Loads remote form basic information from local table.
 *
 * @param $uuid
 *
 * @return mixed
 * @see os2forms_webform_sharing_repo_load_webforms
 */
function os2forms_webform_sharing_repo_load_webform($uuid) {
  $webforms = os2forms_webform_sharing_repo_load_webforms(array($uuid));
  if (!empty($webforms) && is_array($webforms)) {
    return reset($webforms);
  }
}

/**
 * Loads remote forms basic information from local table.
 *
 * @param array $uuids
 * @param array $conditions
 *
 * @return mixed
 */
function os2forms_webform_sharing_repo_load_webforms($uuids = array(), $conditions = array()) {
  $query = db_select('os2forms_webform_sharing_repo_webforms', 'f')
    ->fields('f');

  if (!empty($uuids)) {
    $query->condition('uuid', $uuids, 'IN');
  }
  if (!empty($conditions)) {
    foreach ($conditions as $contidionKey => $condition) {
      $query->condition($contidionKey, $condition);
    }
  }

  $result = $query->execute()->fetchAll();

  return $result;
}

/**
 * Inserts or updates remote webform information.
 *
 * @param $webform
 */
function os2forms_webform_sharing_repo_insert_update_webform($webform) {
  $fields['uuid'] = $webform->uuid;
  $fields['title'] = $webform->title;
  $fields['rnid'] = $webform->nid;
  $fields['ruid'] = $webform->uid;
  $fields['readonly'] = ($webform->readonly) ? $webform->readonly : 0;
  $fields['changed'] = $webform->changed;

  if (os2forms_webform_sharing_repo_load_webform($webform->uuid)) {
    db_update('os2forms_webform_sharing_repo_webforms')
      ->fields($fields)
      ->condition('uuid', $webform->uuid)
      ->execute();
  }
  else {
    db_insert('os2forms_webform_sharing_repo_webforms')
      ->fields($fields)
      ->execute();
  }
}

/**
 * Maps the remote form entity with the nid of a local node.
 *
 * @param $uuid
 * @param $nid
 */
function os2forms_webform_sharing_repo_map_webform($uuid, $nid) {
  db_update('os2forms_webform_sharing_repo_webforms')
    ->fields(array('lnid' => $nid))
    ->condition('uuid', $uuid)
    ->execute();
}

/**
 * Truncates table os2forms_webform_sharing_repo_webforms.
 */
function os2forms_webform_sharing_repo_clear_webforms() {
  db_truncate('os2forms_webform_sharing_repo_webforms')->execute();
}

/**
 * Truncates table os2forms_webform_sharing_repo_users.
 */
function os2forms_webform_sharing_repo_clear_users() {
  db_truncate('os2forms_webform_sharing_repo_users')->execute();
}
