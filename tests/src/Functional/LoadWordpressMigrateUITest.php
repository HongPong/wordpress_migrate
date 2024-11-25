<?php

declare(strict_types=1);

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
   *
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  protected function setUp(): void {
    parent::setUp();
    $this->user = $this->drupalCreateUser(['administer site configuration', 'access administration pages',
      'administer migrations', 'view migration messages', 'migrate wordpress blogs', 'access site reports',
    ]);
    $this->drupalLogin($this->user);
  }

  /**
   * Tests that the home page loads with a 200 response.
   *
   * @throws \Behat\Mink\Exception\ExpectationException
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  public function testLoad(): void {
    $this->drupalGet(Url::fromRoute('<front>'));
    $assert = $this->assertSession();
    $assert->statusCodeEquals(200);

    $account = $this->drupalCreateUser(['administer site configuration', 'access administration pages',
      'administer migrations', 'view migration messages', 'migrate wordpress blogs', 'access site reports',
    ]);
    $this->drupalLogin($account);

    $this->drupalGet('admin');
    $assert->statusCodeEquals(200);

    $this->drupalGet('admin/structure');
    $assert->statusCodeEquals(200);

    $this->drupalGet('admin/structure/migrate');

    // $assert->buttonExists("Add import from WordPress");
    // Not working yet on Drupal 11
    $assert->statusCodeEquals(200);

    $this->drupalGet('admin/structure/migrate/wordpress_migrate');
    $assert->pageTextContains("This wizard supports importing into your Drupal site");
    $assert->statusCodeEquals(200);

    // $this->drupalGet('admin/reports/dblog');
    // $assert->statusCodeEquals(200);
  }

}
