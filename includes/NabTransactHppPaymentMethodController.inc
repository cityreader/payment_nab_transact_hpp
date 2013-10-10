<?php

/**
 * A basic payment method controller.
 *
 * This controller always executes payments successfully, does not require
 * payment-specific information and does not transfer any money. It can be
 * useful to create a "collect on delivery" payment method, for instance.
 */
class NabTransactHppPaymentMethodController extends PaymentMethodController {
  public $billing = array('postal_address', 'email_address', 'telephone');

  public $payment_method_configuration_form_elements_callback = 'payment_nab_transact_hpp_payment_method_configuration_form_elements';

  public $payment_configuration_form_elements_callback = 'payment_nab_transact_hpp_payment_configuration_form_elements';

  public function __construct() {
    $this->title = t('Pay by credit card at NAB');
    $this->description = t('NAB Transact hosted payment gateway.');
    $this->controller_data_defaults = array(
  //    'message' => '',
  //    'status' => PAYMENT_STATUS_SUCCESS,
  //    'text_format' => '',
      'mode' => 'test',
      'vendor_name' => 'NAB0010',
      'payment_alert' => '',
      'privacy_policy' => '',
      'refund_policy' => '',
      'gst_rate' => '10',
      'gst_added' => 1,
      'return_link_text' => t('Click here to complete your payment.'),
      'return_link_url' => '',
    );
  }

  /**
   * Implements PaymentMethodController::validate().
   */
  function validate(Payment $payment, PaymentMethod $payment_method, $strict) {
    parent::validate($payment, $payment_method, $strict);
  }

  /**
   * Implements PaymentMethodController::execute().
   */
  public function execute(Payment $payment) {
    // $payment->setStatus(new PaymentStatusItem(''));
    entity_save('payment', $payment);

    $_SESSION['payment_nab_transact_hpp_pid'] = $payment->pid;

    // Todo: The total amount of $0 is not supposed to happen?
    if ($payment->totalAmount(TRUE) == 0) {
      drupal_goto($payment->method->controller_data['return_link_url']);
    }
    else {
      drupal_goto('payment_nab_transact_hpp/redirect/' . $payment->pid);
    }
  }

  /**
   * Returns the API server URL.
   *
   * @throws InvalidArgumentException
   *
   * @param int $server
   *   self::SERVER_PRODUCTION or self::SERVER_TESTING.
   *
   * @return string
   */
  public function serverURL($mode) {
    return ($mode == 'live') ? PAYMENT_NAB_TRANSACT_HPP_URL_LIVE : PAYMENT_NAB_TRANSACT_HPP_URL_TEST;
  }

  /**
   * Sets up redirect POST data.
   *
   * @param Payment $payment
   *
   * @return array
   *   Keys are POST parameter names, values are values.
   */
  public function redirectData(Payment $payment) {
    $controller_data = $payment->method->controller_data;

    // Get NAB to callback to us as well. Must be port 80.
    $reply_url = url('payment_nab_transact_hpp/complete/' . $payment->pid, array(
      'query' => array(
        'bank_reference' => '',
        'card_type' => '',
        'payment_amount' => '',
        'payment_date' => '',
        'payment_number' => '',
        'remote_ip' => '',
      ),
      'absolute' => TRUE,
    ));
    $return_url = url($controller_data['return_link_url'], array(
      'query' => array(
        'bank_reference' => '',
      ),
      'absolute' => TRUE,
    ));

    $information = paymentbilling_extract($payment);
    $data = array(
//    'AMOUNT' => currency_load($payment->currency_code)->roundAmount($payment->totalAmount(TRUE)),

      'privacy_policy' => url($controller_data['privacy_policy'], array('absolute' => TRUE)),
      'refund_policy' => url($controller_data['refund_policy'], array('absolute' => TRUE)),
      'vendor_name' => $controller_data['vendor_name'],
      'payment_alert' => $controller_data['payment_alert'],
      'payment_reference' => $payment->pid,
      'gst_rate' => $controller_data['gst_rate'],
      'gst_added' => $controller_data['gst_added'] ? 'true' : 'false',
      'reply_link_url' => $reply_url,
      'return_link_text' => $controller_data['return_link_text'],
      'return_link_url' => $return_url,
      'receipt_address' => $information['email_address']['email'],
      'Name' => $information['postal_address']['name_line'],
      'Address' => $information['postal_address']['thoroughfare'],
      'Address2' => $information['postal_address']['premise'],
      'City' => $information['postal_address']['locality'],
      'State' => $information['postal_address']['administrative_area'],
      'Postcode' => $information['postal_address']['postal_code'],
      'Country' => $information['postal_address']['country'],
      'Email' => $information['email_address']['email'],
      'Telephone' => $information['telephone']['safe_value'],
      'print_zero_qty' => 'FALSE',
      'information_fields' => 'Name,Address,Address2,City,State,Postcode,Country,Email,Telephone',
      'suppress_field_names' => 'Address2',
    );
      // throw new Exception('test');
    // Add line items.
    foreach (array_values($payment->line_items) as $line_item) {
      $data[$line_item->description] = $line_item->quantity . ',' . $line_item->unitAmount(TRUE);
    }
    drupal_alter('payment_nab_transact_hpp_redirect_data', $data, $payment);
    return $data;
  }

  /**
   * Gets NAB feedback data.
   *
   * @return array
   */
  function feedbackData() {
    $data = array_merge($_GET, $_POST);
    unset($data['q']);
    return $data;
  }

  function processFeedback($data, Payment $payment) {
    $status_success = new PaymentStatusItem(PAYMENT_STATUS_SUCCESS);
    $payment->setStatus($status_success);
      
    PAYMENT_STATUS_FAILED;
    
  }
}
