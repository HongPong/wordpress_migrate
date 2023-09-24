<?php

declare(strict_types = 1);

namespace Drupal\Tests\wordpress_migrate\Functional;

use Drupal\Core\Url;
use Drupal\Tests\BrowserTestBase;

/**
 * Simple test to ensure that main page loads with module enabled.
 *
 * @group wordpress_migrate
 */
final class LoadWordpressMigrateUITest extends BrowserTestBase {

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'wordpress_migrate',
    'wordpress_migrate_ui',
    'migrate_plus',
    'migrate_tools',
    'ctools',
    'file',
    'taxonomy',
  ];

  /**
   * A user with permission to administer site configuration.
   *
   * @var \Drupal\user\UserInterface
   */
  protected $user;

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'claro';

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();
    $this->user = $this->drupalCreateUser(['administer site configuration']);
    $this->drupalLogin($this->user);
  }

  /**
   * Tests that the home page loads with a 200 response.
   */
  public function testLoad(): void {
    $this->drupalGet(Url::fromRoute('<front>'));
    $this->assertSession()->statusCodeEquals(200);
  }

}
