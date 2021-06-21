<?php

/**
 * @file
 * Forms for OpenID Connect.
 */

/**
 * Form builder: Log in with an external account.
 */
function openid_connect_login_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'openid_connect', 'includes/openid_connect.forms');

  foreach (openid_connect_get_plugins(TRUE) as $client) {

    $form['tipo_identificacion'] = [
        '#type' => 'select',
        '#title' => t('Tipo de identificación'),
        '#options' => [
          'CC' => t('Cédula de ciudadanía'),
          'EM' => t('Correo electrónico'),
        ],
    ];

    $form['identificacion'] = array(
        '#type' => 'textfield',
        '#title' => t('Identificación'),
    );

    $form['openid_connect_client_' . $client['name'] . '_login'] = array(
      '#type' => 'submit',
      '#value' => t('Entrar', array('!client_title' => $client['title'])),
      '#name' => $client['name'],
      '#nameType' => 'login',
    );

    $form['openid_connect_client_' . $client['name'] . '_register'] = array(
      '#type' => 'submit',
      '#value' => t('Registrar', array('!client_title' => $client['title'])),
      '#name' => $client['name'],
      '#nameType' => 'register',
    );
    
  }

  return $form;
}

/**
 * Form submit handler: Log in with an OpenID Connect login provider.
 */
function openid_connect_login_form_submit(&$form, &$form_state) {

  if (validateForm($form,$form_state)) {
      // There were errors, do not continue.
      return $form;
  }
  openid_connect_save_destination();
  $client_name = $form_state['triggering_element']['#name'];
  $type = $form_state['triggering_element']['#nameType'];
  $client = openid_connect_get_client($client_name);
  $scopes = openid_connect_get_scopes();
  $_SESSION['openid_connect_op'] = 'login';
  $login_hint = $form_state['values']['tipo_identificacion'].','.$form_state['values']['identificacion'];
  $client->authorize($scopes, $type, $login_hint);
}

function validateForm($form, $form_state){
  if (empty($form_state['values']['identificacion'])) {
    form_set_error('identificacion', t('El campo identificación es requerido.'));
    return true;
  }
      

  if($form_state['values']['tipo_identificacion'] == 'CC' && !is_numeric($form_state['values']['identificacion'])){
    form_set_error('identificacion', t('El campo identificación debe ser númerico cuando se selecciona cédula de ciudadanía.'));
    return true;
  }
      
    
  if($form_state['values']['tipo_identificacion'] == 'EM' && !filter_var($form_state['values']['identificacion'], FILTER_VALIDATE_EMAIL)){
    form_set_error('identificacion', t('El campo identificación no contiene un correo electrónico valido.'));
    return true;
  }

  return false;
}

/**
 * Form builder: Connect an external account to your existing Drupal account.
 */
function openid_connect_connect_form($form, &$form_state, $account) {
  form_load_include($form_state, 'inc', 'openid_connect', 'includes/openid_connect.forms');

  $form_state['account'] = $account;

  $clients = openid_connect_get_plugins(TRUE);

  $form['help'] = array(
    '#prefix' => '<p class="description">',
    '#suffix' => '</p>',
  );

  if (empty($clients)) {
    $form['help']['#markup'] = t('No external account providers are available.');

    return $form;
  }
  elseif ($GLOBALS['user']->uid == $account->uid) {
    $form['help']['#markup'] = t('You can connect your account with these external providers.');
  }

  $connected_accounts = openid_connect_get_connected_accounts($account);

  foreach ($clients as $client) {
    $form[$client['name']] = array(
      '#type' => 'fieldset',
      '#title' => t('Provider: @title', array('@title' => $client['title'])),
    );
    $fieldset = &$form[$client['name']];
    $connected = isset($connected_accounts[$client['name']]);
    $fieldset['status'] = array(
      '#type' => 'item',
      '#title' => t('Status'),
      '#markup' => t('Not connected'),
    );
    if ($connected) {
      $fieldset['status']['#markup'] = t('Connected as %sub', array(
        '%sub' => $connected_accounts[$client['name']],
      ));
      $fieldset['openid_connect_client_' . $client['name'] . '_disconnect'] = array(
        '#type' => 'submit',
        '#value' => t('Disconnect from !client_title', array('!client_title' => $client['title'])),
        '#name' => 'disconnect__' . $client['name'],
      );
    }
    else {
      $fieldset['status']['#markup'] = t('Not connected');
      $fieldset['openid_connect_client_' . $client['name'] . '_connect'] = array(
        '#type' => 'submit',
        '#value' => t('Connect with !client_title', array('!client_title' => $client['title'])),
        '#name' => 'connect__' . $client['name'],
        '#access' => $GLOBALS['user']->uid == $account->uid,
      );
    }
  }

  return $form;
}

/**
 * Submit callback for the connect account form.
 *
 * Connect the current user's Drupal account to an external provider.
 */
function openid_connect_connect_form_submit(&$form, &$form_state) {
  list($op, $client_name) = explode('__', $form_state['triggering_element']['#name'], 2);

  if ($op === 'disconnect') {
    //openid_connect_disconnect_account($form_state['account'], $client_name);
    //$client = openid_connect_get_client($client_name);
    //drupal_set_message(t('Account successfully disconnected from @client.', array('@client' => $client->getLabel())));
    global $user; 
    
    $id_token = $_SESSION['token_tmp'];
    $client = openid_connect_get_client($client_name);
    $response = $client->endSession($id_token);

    module_invoke_all('user_logout', $user);
    session_destroy();
    drupal_goto();
    
    return;
  }

  global $user;

  if ($user->uid !== $form_state['account']->uid) {
    drupal_set_message(t("You cannot connect another user's account."), 'error');

    return;
  }

  openid_connect_save_destination();

  $client = openid_connect_get_client($client_name);
  $scopes = openid_connect_get_scopes();
  $_SESSION['openid_connect_op'] = $op;
  $_SESSION['openid_connect_connect_uid'] = $user->uid;
  $client->authorize($scopes);
}
