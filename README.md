# Wordpress Migrate

## Table of contents

- Introduction
- Requirements
- Installation
- Migrate via User Interface Wizard
- Importing Image Assets
- API
- Drush 12+ Support (In development)
- Support
- Similar Modules
- Credits

## Introduction
The WordPress Migrate module provides tools for setting up migration processes
from the WordPress blog to a Drupal 9/10 site. By providing a few configuration
settings and a pointer to an XML export file, migration configuration entities
will be generated which can then be executed or otherwise managed with the
Migrate Tools module.

This module has been developed since 2010 to reliably import WordPress sites into
Drupal.

See the [documentation page](https://drupal.org/docs/extending-drupal/contributed-modules/contributed-module-documentation/wordpress-migrate),
the [FAQ](https://drupal.org/docs/extending-drupal/contributed-modules/contributed-module-documentation/wordpress-migrate/wordpress-migrate-faq),
and [WordPress and Drupal terminology and concepts](https://drupal.org/docs/extending-drupal/contributed-modules/contributed-module-documentation/wordpress-migrate/wordpress-and-drupal-terminology-and-concepts).

## Requirements

The wordpress_migrate and wordpress_migrate_ui modules require [migrate_plus](https://drupal.org/project/migrate_plus) ~6.0, [ctools](https://drupal.org/project/ctools) 3.x or 4.x, and [pathauto](https://drupal.org/project/pathauto) above 1.13.

## Installation

Using Composer 2, install:

```bash
composer require 'drupal/wordpress_migrate:^3.0@alpha'
````

## Migrate via User Interface Wizard

Enabling the WordPress Migrate UI module. This creates an
"Add import from WordPress" button on the migrate_tools UI at **/admin/structure/migrate** . From there a wizard
prompts you for the configuration options.

Enter 'Base url of the WordPress site' into the first stage of the wizard
to automatically generate permalinks to the new content nodes.

The configurations will be created for the posts, pages, attachments, authors
and comments from the source data as a _migration group_. Then they can be
imported and reversed as needed.

## Importing Image Assets

For the new importation of attached images including post thumbnails (also known as featured images) you should:

- Assign an image field for these assets in the migrate process.
- Before importing the content posts, import the media assets.
- Import the media assets.

See issue for more information and customization:
<https://drupal.org/project/wordpress_migrate/issues/2742269>

Important: While image assets can be imported to the Drupal filesystem,
the paths of inline images from the WordPress body are not rewritten.
See issue: <https://drupal.org/project/wordpress_migrate/issues/2742279>

## Security Note

Existing users (keyed by email address) will not be overwritten if imported.
new [access checks](https://www.drupal.org/node/3201242) are allowed in entity lookups during the migrate
process.

Our module assumes the operator of the migration is an administrator who has access to all entities. Also,
this module can create new users (WordPress post authors) which are set to "active" so be sure to review your users
after the migration is completed.

Importing posts and comments can potentially include harmful JavaScript snippets or other malicious materials. We
recommend you carefully review all imported content for security issues from unsanitized markup.

The module can be disabled and its data deleted after your migration is complete. It does not need to stay enabled.

## API

You may also programmatically configure a set of WordPress migrations by
constructing a configuration array and passing it to the generator:

```php
use Drupal\wordpress_migrate\WordPressMigrationGenerator;

$configuration = [
 'file_uri' => '/var/data/my_wp_export.xml',
 'base_url' => 'https://myoriginalblogurl.com',
 'group_id' => 'old_blog',
 'prefix' => 'blog_',
 'default_author' => 'editor_account',
 'tag_vocabulary' => 'tags',
 'category_vocabulary' => 'wp_categories',
 'post' => [
   'type' => 'article',
   'text_format' => 'restricted_html',
 ],
 'page' => [
   'type' => 'page',
   'text_format' => 'full_html',
 ],
];
$generator = new WordPressMigrationGenerator($configuration);
$generator->createMigrations();
```

## Drush 12+ Support (In development)

See original
[issue](https://drupal.org/project/wordpress_migrate/issues/2955644)
for Drush 9/10 and new issue for [Drush 12](https://drupal.org/project/wordpress_migrate/issues/3489516).

Drush 8 code has been removed as it is no longer supported.

### Support, known issues and plans

- Your support, questions and contributions are welcome.
  Please try to provide example files to help reproduce errors and notices:
  <https://drupal.org/project/issues/wordpress_migrate>
- **Plan for 8.x-3.x beta release:** (The current main plan)
  <https://drupal.org/project/wordpress_migrate/issues/2904990>
- Comment migration may need to set a body text format:
  <https://drupal.org/project/wordpress_migrate/issues/2742311>
- Drush 12+ support:
  <https://drupal.org/project/wordpress_migrate/issues/3489516>
- Random strings in taxonomies:
  <https://drupal.org/project/wordpress_migrate/issues/2974024>
- Rewrite local link/image references in content:
  <https://drupal.org/project/wordpress_migrate/issues/2742279>
- Extract and save blog metadata:
  <https://drupal.org/project/wordpress_migrate/issues/2742287>

### Similar projects

**[WordPress Migrate SQL](https://www.drupal.org/project/wordpress_migrate_sql):** Enables customized migrations based on WordPress SQL sites,
allowing migration of complex WordPress sites, using a SQL source.
**[wp_migrate](https://www.drupal.org/project/wp_migrate)** is another module which is compatible with up to Drupal 9.

## Credits

**Current co-maintainer:**

- [HongPong](https://drupal.org/u/HongPong)

Originally developed for Drupal 7 and 8 by [mikeryan](https://drupal.org/u/mikeryan).

**Committers and active volunteers have included:**
somersoft, lomasr, chaitanya17, felribeiro, maccath, MaskyS,
mrmikedewolf, Darren Shelley, dwillems, othermachines, ohthehugemanatee,
ezeedub, grasmash, bdone, queenvictoria, ksenzee, ptaff, pverrier,
xurizaemon, hekele, aaron, emarchak, wizonesolutions, baltowen, msielski, ressa,
vlad.dancer, nitapawar, phjou, el7cosmos, batonac, stargayte, darchuletajr,
e.ruiter, ankshetty, i.vuchkov, sahana16081996, marktonino, john_b, frederickjh,
damienmckenna, splash112, caspervoogt, ivrh, tabestan, bserem, apmsooner, dinarcon,
mithun-a-sridharan, ryumaou, tolstoydotcom, uridrupal, lobodakyrylo, vaidas_a, ryumaou
