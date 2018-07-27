<?php

/**
 * Webform clone batch definition.
 *
 * @param $uuids
 *
 * @return array
 */
function os2forms_webform_sharing_batch_clone($uuids) {
  drupal_set_message(t('Preparing to clone @num node(s).', array('@num' => count($uuids))));

  $operations = array();
  for ($i = 0; $i < count($uuids); $i++) {
    $operations[] = array(
      'os2forms_webform_sharing_batch_clone_op',
      array(
        $uuids[$i],
        t('(Cloning operation @operation)', array('@operation' => $i)),
      )
    );
  }
  $batch = array(
    'operations' => $operations,
    'title' => t('Processing cloning'),
    'init_message' => t('Cloning is starting.'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Cloning has encountered an error.'),
    'finished' => 'os2forms_webform_sharing_batch_clone_finished',
  );

  return $batch;
}

/**
 * Webform clone batch operation.
 * Clones the webform locally.
 *
 * @param $uuid
 * @param $operation_details
 * @param $context
 */
function os2forms_webform_sharing_batch_clone_op($uuid, $operation_details, &$context) {
  // Load webform basic data we are about to clone.
  $webform = os2forms_webform_sharing_repo_load_webform($uuid);

  // Load localwebform (if we have one already).
  $originalLocalWebform = os2forms_webform_sharing_load_webform($uuid);

  // Report progress.
  $context['message'] = t('Cloning node %title', array('%title' => $webform->title));

  // Actual webservice call.
  $data = os2forms_webform_sharing_get_webform_node_export_by_uuid($uuid);

  // We got the node export data.
  if (strpos($data, 'node_export_serialize') === 0) {
    // Prepare and create a new node.
    $import = node_export_import($data);
    if ($import['success']) {
      // We expect only one node to be imported.
      $nid = reset($import['nids']);

      // Keep the original author, if present.
      if ($originalLocalWebform) {
        $localWebform = node_load($nid);
        $localWebform->uid = $originalLocalWebform->uid;

        // Saving the node without updating changed timestamp.
        node_export_save($localWebform);
      }

      $context['results'][] = array(
        'message' => t('Webform "@title" imported into <a href="@link">@title</a>.', array(
          '@title' => $webform->title,
          '@link' => url('node/' . $nid)
        )),
        'type' => 'status'
      );

      $webform->nid = $nid;
      os2forms_webform_sharing_repo_map_webform($uuid, $nid);
    }
  }
  else {
    // Report the problem.
    $context['results'][] = array(
      'message' => t('Webform "@title" import failed. Message: @message', array(
        '@title' => $webform->title,
        '@message' => $data,
      )),
      'type' => 'warning'
    );
  }


}

/**
 * Batch finished callback for clone operation.
 *
 * @param $success
 * @param $results
 * @param $operations
 */
function os2forms_webform_sharing_batch_clone_finished($success, $results, $operations) {
  if ($success) {
    // Counting successful.
    $successCount = 0;
    foreach ($results as $result) {
      if ($result['type'] === 'status') {
        $successCount++;
      }
    }

    drupal_set_message(t('@successCount out of @totalCount node(s) have been cloned.', array(
      '@successCount' => $successCount,
      '@totalCount' => count($results)
    )));

    foreach ($results as $result) {
      drupal_set_message($result['message'], $result['type']);
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(
      t('An error occurred while processing {
      @operation
} with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      ),
      'error'
    );
  }
}

/**
 * Webform commit batch definition.
 *
 * @param $uuids
 *
 * @return array
 */
function os2forms_webform_sharing_batch_commit($uuids) {
  drupal_set_message(t('Preparing to commit @num node(s) . ', array('@num' => count($uuids))));

  $operations = array();
  for ($i = 0; $i < count($uuids); $i++) {
    $operations[] = array(
      'os2forms_webform_sharing_batch_commit_op',
      array(
        $uuids[$i],
        t('(Commit operation @operation)', array('@operation' => $i)),
      )
    );
  }
  $batch = array(
    'operations' => $operations,
    'title' => t('Processing commit'),
    'init_message' => t('Commit is starting . '),
    'progress_message' => t('Processed @current out of @total . '),
    'error_message' => t('Commit has encountered an error . '),
    'finished' => 'os2forms_webform_sharing_batch_commit_finished',
  );

  return $batch;
}

/**
 * Webform commit batch operation.
 * Pushed the webform to the remote server, upon success updated the local entry with the received node ID.
 *
 * @param $uuid
 * @param $operation_details
 * @param $context
 */
function os2forms_webform_sharing_batch_commit_op($uuid, $operation_details, &$context) {
  // Load webform basic data we are about to commit.
  $webform = os2forms_webform_sharing_load_webform($uuid);

  // Report progress.
  $context['message'] = t('Committing node @title', array('@title' => $webform->title));

  // Exporting with serialize formatter.
  $export = node_export(intval($webform->nid), 'serialize');

  if (!empty($export['output'])) {
    $data = $export['output'];

    // Actual webservice call.
    $result = os2forms_webform_sharing_post_webform_node_export($data);

    // Operation was successful.
    if ($result->nid) {
      os2forms_webform_sharing_repo_insert_update_webform($result);
      os2forms_webform_sharing_repo_map_webform($result->uuid, $webform->nid);
      $context['results'][] = array(
        'message' => t('Webform "@title" is successfully committed, and its remote nid is @rnid', array(
          '@title' => $webform->title,
          '@rnid' => $result->nid
        )),
        'type' => 'status'
      );
    }
    else {
      $context['results'][] = array(
        'message' => t('Webform "@title" commit failed, message: @message', array(
          '@title' => $webform->title,
          '@message' => implode(' ', $result)
        )),
        'type' => 'warning'
      );
    }
  }
}

/**
 * Batch finished callback for commit operation.
 *
 * @param $success
 * @param $results
 * @param $operations
 */
function os2forms_webform_sharing_batch_commit_finished($success, $results, $operations) {
  if ($success) {
    // Counting successful.
    $successCount = 0;
    foreach ($results as $result) {
      if ($result['type'] === 'status') {
        $successCount++;
      }
    }

    drupal_set_message(t('@successCount out of @totalCount node(s) have been committed.', array(
      '@successCount' => $successCount,
      '@totalCount' => count($results)
    )));

    foreach ($results as $result) {
      drupal_set_message($result['message'], $result['type']);
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(
      t('An error occurred while processing {
      @operation
} with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      ),
      'error'
    );
  }
}

/**
 * Webform unlink batch definition.
 *
 * @param $uuids
 *
 * @return array
 */
function os2forms_webform_sharing_batch_unlink($uuids) {
  drupal_set_message(t('Preparing to unlink @num node(s) . ', array('@num' => count($uuids))));

  $operations = array();
  for ($i = 0; $i < count($uuids); $i++) {
    $operations[] = array(
      'os2forms_webform_sharing_batch_unlink_op',
      array(
        $uuids[$i],
        t('(Commit operation @operation)', array('@operation' => $i)),
      )
    );
  }
  $batch = array(
    'operations' => $operations,
    'title' => t('Processing unlinking'),
    'init_message' => t('Unlinking is starting . '),
    'progress_message' => t('Processed @current out of @total . '),
    'error_message' => t('Unlinking has encountered an error . '),
    'finished' => 'os2forms_webform_sharing_batch_unlink_finished',
  );

  return $batch;
}

/**
 * Webform unlink batch operation.
 * Unlinks webform from remoted server by changing its uuid, so that it becomes detached.
 *
 * @param $uuid
 * @param $operation_details
 * @param $context
 */
function os2forms_webform_sharing_batch_unlink_op($uuid, $operation_details, &$context) {
  // Load webform basic data we are about to commit.
  $webform = os2forms_webform_sharing_load_webform($uuid);
  $originalUuid = $webform->uuid;

  // Report progress.
  $context['message'] = t('Unlinking node % title', array(' % title' => $webform->title));

  // Unsetting uuid and saving node, saving node will automatically generate new uuid.
  unset($webform->uuid);
  node_save($webform);

  if ($webform->uuid !== $originalUuid) {
    // Remove connection from remote form to a current nid.
    os2forms_webform_sharing_repo_map_webform($originalUuid, NULL);

    $context['results'][] = t('@title is successfully unlinked . ', array(
      '@title' => $webform->title
    ));
  }
}

/**
 * Batch finished callback for unlink operation.
 *
 * @param $success
 * @param $results
 * @param $operations
 */
function os2forms_webform_sharing_batch_unlink_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(t('@count node(s) have been unlinked . ', array('@count' => count($results))));
    foreach ($results as $result) {
      drupal_set_message($result);
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(
      t('An error occurred while processing {
      @operation
} with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      ),
      'error'
    );
  }
}