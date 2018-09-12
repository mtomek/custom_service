<?php

/**
 * @file
 * Menu builder functions for Custom service.
 */


/**
 * Form builder for /service/add form
 * @param $form
 * @param $form_state
 * @return array $form
 */
function custom_service_page_service_add_form($form, &$form_state) {
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => 'ФИО',
    '#default_value' => '',
    '#description' => 'Введите Фамилию, Имя, Отчество',
    '#required' => TRUE,
  );
  $form['phone'] = array(
    '#type' => 'textfield',
    '#title' => 'Телефон',
    '#default_value' => '',
    '#description' => 'Ваш номер телефона',
    '#required' => TRUE,
  );
  $form['mail'] = array(
    '#type' => 'textfield',
    '#title' => 'E-mail',
    '#default_value' => '',
    '#description' => 'Ваша электронная почта',
    '#required' => TRUE,
  );
  $form['city'] = array(
    '#type' => 'select',
    '#title' => 'Город',
    '#options' => array(
      'moscow' => 'Москва',
      'minsk' => 'Минск',
      'kiev' => 'Киев',
    ),
    '#required' => TRUE,
    '#default_value' => 'moscow',
    '#description' => 'Ваш город',
  );
  $form['file_inn'] = array(
    '#type' => 'file',
    '#title' => 'ИНН',
    '#description' => 'Скан ИНН в формате JPG/PNG ',

  );
  $form['actions']['send'] = array(
    '#type' => 'submit',
    '#value' => t('Отправить'),
    '#weight' => 15,
   );
//  $form['actions']['clear'] = array(
//    '#type' => 'submit',
//    '#value' => t('Очистить'),
//    '#weight' => 15,
//  );
  $form['#validate'][] = 'custom_service_page_service_add_form_validate';
  return $form;
}

/**
 * Form validate for /service/add
 * @param $form
 * @param $form_state
 */
function custom_service_page_service_add_form_validate($form, &$form_state) {
  //form_set_error('name', 'Title cannot be "test"');

  //fio validation
  if (preg_match('/^-?[A-Z a-z а-яёї А-ЯЁЇ]+$/iu', $form_state['values']['name']) == 0) {
    form_set_error('name', 'В ФИО не может быть цыфр и знаков пунктуации');
  }
  $count_names = db_select('custom_service_list', 'cs')
    ->condition('cs.name', $form_state['values']['name'])
    ->countQuery()
    ->execute()
    ->fetchField();
  if ($count_names>0) {
    form_set_error('name', 'Указанное имя существует в базе. ');
  }

  //phone validation
  $is_correct_phone = (bool)preg_match('#^[-+0-9()\s]{1,19}+$#', $form_state['values']['phone']);
  if (!$is_correct_phone) {
    form_set_error('phone', 'Поле Телефон может только содержать цыфры 0-9, скобки (), 
    знаки + и -. Длина не может быть больше 19 символов ');
  }

  $phone = str_replace(array('+', ' ', '(' , ')', '-'), '', $form_state['values']['phone']);

  $count_phones = db_select('custom_service_list', 'cs')
    ->condition('cs.phone', $phone)
    ->countQuery()
    ->execute()
    ->fetchField();

  if ($count_phones>0) {
    form_set_error('phone', 'Указанный телефон существует в базе. ');
  }

  //mail validation
  if (!filter_var($form_state['values']['mail'], FILTER_VALIDATE_EMAIL)) {
    form_set_error('mail', 'Неправильный формат E-mail');
  }

  $count_mails = db_select('custom_service_list', 'cs')
    ->condition('cs.mail', $form_state['values']['mail'])
    ->countQuery()
    ->execute()
    ->fetchField();

  if ($count_mails>0) {
    form_set_error('mail', 'Указанный E-mail существует в базе. ');
  }

  //file_inn validation
  $validators = array(
    'file_validate_is_image' => array(), // Проверка, действительно ли файл является изображением
    'file_validate_extensions' => array('png jpg jpeg'), // Проверка на расширения
    'file_validate_size' => array(1 * 1024 * 1024), // Проверка на размер файла (максимум 1mb)
  );
  // Загружаем файл в sites/default/files (по умолчанию)
  $file_inn_storage = variable_get('custom_service_image_folder', 'public://');
  if ($file = file_save_upload('file_inn', $validators, $file_inn_storage )) {
    $form_state['values']['file_inn'] = $file; // передаём информацию о файле в ф-ю mymodule_form_submit()
  }
  else {
    form_set_error('file_inn', 'ИНН не загружен');
  }
}

/**
 * Form submit callback.
 */
function custom_service_page_service_add_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  // вызывается хук mymodule_service_alter, которому в качестве параметра передаются данные (для их возможного изменения).
  drupal_alter('custom_service_service', $values);

  $file = $form_state['values']['file_inn'];
  $file->status = FILE_STATUS_PERMANENT; // Изменяем статус файла на "Постоянный"
  file_save($file); // Сохраняем новый статус

  $values['file_inn_image'] = $file;

  $account = new stdClass();
  $account->name = $values['name'];
  $account->mail = $values['mail'];
  $account->init = $values['mail'];
  $account->pass = user_password();
  $account->roles = array(DRUPAL_AUTHENTICATED_RID => TRUE);
  $account->status = 1;
  user_save($account);
  if ($account->uid) {
    $values['uid']=$account->uid;
  }

  //данные отправляются как POST-запрос на тестовый адрес с полями phone, name, mail и city (валидации)
  $url=variable_get('custom_service_api_address', '');

  $result = drupal_http_request($url, array(
    'method' => 'POST',
    'headers' => array(
      'Content-Type' => 'application/x-www-form-urlencoded',
    ),
    'data' => drupal_http_build_query(array(
      'name' => $values['name'],
      'phone' => $values['phone'],
      'mail' => $values['mail'],
      'city' => $values['city'],
    )),
  ));
  $result_data=json_decode($result->data);
  $values['status'] = ($result_data->status=='success')?1:0;
  $values['uuid'] = $result_data->uuid;

  $file_to_zip = array(
    drupal_realpath($values['file_inn_image']->uri),
  );
  $destination = drupal_realpath(variable_get('custom_service_zip_folder', 'public://')) . '/' . $values['file_inn_image']->filename . '.zip';
  //if true, good; if false, zip creation failed
  $created_zip = add_images_to_zip(
    $file_to_zip,
    $destination
  );

  $uri = variable_get('custom_service_zip_folder', 'public://') . $values['file_inn_image']->filename . '.zip';
  $filename = $values['file_inn_image']->filename . '.zip';

  $file_zip = new stdClass;
  $file_zip->uid = 1;
  $file_zip->filename = $filename;
  $file_zip->uri = $uri;
  $file_zip->filemime = 'application/zip';
  $file_zip->filesize = filesize($uri);
  $file_zip->status = 1;

  file_save($file_zip);

  $values['file_inn']=$file_zip;

  $id = db_insert('custom_service_list')
    ->fields(array(
      'name' => $values['name'],
      'phone' => $values['name'],
      'mail' => $values['mail'],
      'city' => $values['city'],
      'file_inn' => $values['file_inn']->fid,
      'file_inn_image' => $values['file_inn_image']->fid,
      'uid' => $values['uid'],
      'status' => $values['status'],
      'uuid' => $values['uuid'],
      'created' => time(),
    ))
    ->execute();
  foreach (module_invoke_all('custom_service_service_insert') as $values) {
    $options[] = $option;
  }

  //вызывается хук mymodule_service_insert, которому в качестве параметра передаются данные

  foreach (module_implements('custom_service_service_insert') as $module) {
    $function = $module . '_custom_service_service_insert';
    $function($values);
  }

  drupal_set_message(t('Form saved'));
}

/**
 * display /service/list
 *
 * @result string
 */
function custom_service_page_service_list() {

  $header = array(
    array('data' => 'Дата', 'field' => 'created'),
    array('data' => 'ФИО', ),
    array('data' => 'Телефон', ),
    array('data' => 'E-mail', ),
    array('data' => 'миниатюра скана ИНН', ),
    array('data' => 'ссылка на ZIP', ),
    array('data' => 'статус', ),
    array('data' => 'UUID', ),
  );
  $cs = db_select('custom_service_list', 'cs')
    ->fields('cs' , array('created', 'name', 'phone',  'mail', 'file_inn_image', 'file_inn', 'status', 'uuid' ))
    ->extend('PagerDefault')
    ->limit(20)
    ->orderBy('cs.created', 'DESC')
    ->execute();

  $rows = array();
  foreach ($cs as $cs_row) {
    $rows[] = array(
      format_date($cs_row->created),
      check_plain($cs_row->name),
      $cs_row->phone,
      $cs_row->mail,
      '<img src="' . image_style_url('thumbnail', file_load($cs_row->file_inn_image)->uri) . '" >',
      l(t('download'), file_create_url(file_load($cs_row->file_inn)->uri)),
      $cs_row->status==1 ? 'success' : 'error',



      $cs_row->uuid,
    );
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');


  return $output;

}


/**
 * Form builder function for module settings.
 */
function custom_service_settings_form($form, &$form_state) {
  $form['custom_service_api_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Custom service api address'),
    '#default_value' => variable_get('custom_service_api_address', 1),
    '#description' => t('Custom service api address'),
  );
  return system_settings_form($form);
}

/**
 * Add images to zip archive
 * @param $files
 * @param $destination
 * @param $overwrite
 * @return bool
 */
function add_images_to_zip(array $files, $destination = '', $overwrite = FALSE) {
  //if the zip file already exists and overwrite is false, return false
  //if(file_exists($destination) ) { return false; }
  //vars
  $valid_files = array();
  //if files were passed in...
  if (is_array($files)) {
    //cycle through each file
    foreach ($files as $file) {
      //make sure the file exists
      if (file_exists($file)) {
        $valid_files[] = $file;
      }
    }
  }
  //if we have good files...
  if (count($valid_files)) {
    //create the archive
    $zip = new ZipArchive();
    if ($zip->open($destination, $overwrite ? ZIPARCHIVE::OVERWRITE : ZIPARCHIVE::CREATE) !== TRUE) {
      return FALSE;
    }
    //add the files
    foreach ($valid_files as $file) {
      $zip->addFile($file, $file);
    }

    //close the zip -- done!
    $zip->close();

    //check to make sure the file exists
    return file_exists($destination);
  }
  else {
    return FALSE;
  }
}