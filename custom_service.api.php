<?php

/**
 * @file
 * API documentation for Custom service module.
 */
/**
 * Change the /service/add form values before saving.
 *
 * Only use this hook to alter existing form values.
 *
 * @param array $values
 *   $values['name'] - string FIO field.
 *   $values['phone'] - string Phone field
 *   $values['$mail'] - string E-mail field
 *   $values['city'] - string City field. Allowed values are 'moscow','minsk','kiev'
 *   $values['file_inn'] - drupal file object
 *
 */
function hook_custom_service_service_alter(&$values) {

}

/**
 * Implements hook custom_service_service_insert.
 *
 * @param array $values
 *
 */
function hook_custom_service_service_insert(&$values) {

}

