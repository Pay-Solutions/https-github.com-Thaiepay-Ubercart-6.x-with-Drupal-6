<?php
// $Id: uc_paysolutions.pages.inc,v 1.1.2.3 2009/04/28 21:03:17 islandusurper Exp $

/**
 * @file
 * paysolutions menu items.
 *
 */

function uc_paysolutions_complete($cart_id = 0) {
  watchdog('paysolutions', 'Receiving new order notification for order !order_id.', array('!order_id' => check_plain($_GET['order'])));

  $order = uc_order_load($_GET['order']);

  if ($order === FALSE || uc_order_status_data($order->order_status, 'state') != 'in_checkout') {
    print t('An error has occurred during payment.  Please contact us to ensure your order has submitted.');
    exit();
  }

  if ($_POST['demo'] != 'Y') {
    $key = $_POST['key'];
    $valid = md5(variable_get('uc_paysolutions_secret_word', 'tango') . $_POST['sid'] . $_POST['merchant_order_id'] . $_POST['total']);
    if (strtolower($key) != strtolower($valid)) {
      uc_order_comment_save($order->order_id, 0, t('Attempted unverified PAYSOLUTIONS completion for this order.'), 'admin');
    }
  }

  $order->billing_street1 = $_POST['street_address'];
  $order->billing_street2 = $_POST['street_address2'];
  $order->city = $_POST['city'];
  $order->billing_postal_code = $_POST['zip'];
  $order->billing_phone = $_POST['phone'];

  $zone_id = db_result(db_query("SELECT zone_id FROM {uc_zones} WHERE zone_code LIKE '%s'", $_POST['state']));
  if (!empty($zone_id)) {
    $order->billing_zone = $zone_id;
  }

  $country_id = db_result(db_query("SELECT country_id FROM {uc_countries} WHERE country_name LIKE '%s'", $_POST['country']));
  if (!empty($zone_id)) {
    $order->billing_country = $country_id;
  }

  if (strtolower($_POST['email']) !== strtolower($order->primary_email)) {
    uc_order_comment_save($order->order_id, 0, t('Customer used a different e-mail address during payment: !email', array('!email' => check_plain($_POST['email']))), 'admin');
  }

  if ($_POST['credit_card_processed'] == 'Y' && is_numeric($_POST['total'])) {
    $comment = t('Paid by !type, thaiepay.com order #!order.', array('!type' => $_POST['pay_method'] == 'CC' ? t('credit card') : t('echeck'), '!order' => check_plain($_POST['order_number'])));
    uc_payment_enter($order->order_id, 'paysolutions', $_POST['total'], 0, NULL, $comment);
  } else {
    drupal_set_message(t('Your order will be processed as soon as your payment clears at thaiepay.com.'));
    uc_order_comment_save($order->order_id, 0, t('!type payment is pending approval at thaiepay.com.', array('!type' => $_POST['pay_method'] == 'CC' ? t('Credit card') : t('eCheck'))), 'admin');
  }

  // Empty that cart...
  uc_cart_empty($cart_id);

  // Save changes to order without it's completion (it will be on finalization step)
  uc_order_save($order);

  $url = 'cart/paysolutions/finalize/'. $order->order_id;

  // Javascript redirect on the finalization page.
  $output = '<script type="text/javascript">window.location = "'. url($url, array('absolute' => TRUE)) .'";</script>';

  // Text link for users without Javascript enabled.
  $output .= l(t('Click to complete checkout.'), $url, array('absolute' => TRUE));

  // 2CO needs page larger than 255 characters to display.
  while (strlen($output) < 255) {
    $output .= '&nbsp;';
  }

  print $output;
  exit();
}

function uc_paysolutions_finalize() {
  $order = uc_order_load(arg(3));

  // Add a comment to let sales team know this came in through the site.
  uc_order_comment_save($order->order_id, 0, t('Order created through website.'), 'admin');

  $output = uc_cart_complete_sale($order, variable_get('uc_new_customer_login', FALSE));

  $page = variable_get('uc_cart_checkout_complete_page', '');

  if (!empty($page)) {
    drupal_goto($page);
  }

  return $output;
}