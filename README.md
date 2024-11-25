# Wordpress Migrate

## Table of contents

- Introduction
- Requirements
- Installation
- Drush Command
- User Interface
- Importing Image Assets
- API
- Documentation
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
"Add import from WordPress" button on the migrate_tools UI at **/admin/structure/migrate** . From there a wizard prompts you
for the configuration options.

Enter 'Base url of the WordPress site' into the first stage of the wizard
to automatically generate permalinks to the new content nodes.

The configurations will be created for the posts, pages, attachments, authors
and comments from the source data as a _migration group_. Then they can be
imported and reversed as needed.

## Importing Image Assets

For the new importation of attached images including post thumbnails
(also known as featured images) you should:

- Assign an image field for these assets in the migrate process.
- Before importing the content posts, import the media assets.
- Import the media assets.

See issue for more information and customization:
<https://drupal.org/project/wordpress_migrate/issues/2742269>

Important: While image assets can be imported to the Drupal filesystem,
the paths of inline images from the WordPress body are not rewritten.
See issue: <https://drupal.org/project/wordpress_migrate/issues/2742279>

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

## Drush Command (deprecated)

This module is not Drush 9+ compatible. See original [issue](https://www.drupal.org/project/wordpress_migrate/issues/2955644) and new issue for [Drush 12](https://www.drupal.org/project/wordpress_migrate/issues/3489516). Previous docs follow:

A single Drush command, `wordpress-migrate-generate`, is provided for generating
WordPress migrations from a few simple options:

Arguments:

```
 file_uri                             Address of the WordPress export file to migrate into Drupal.

Options:
 --group-id=<my_wordpress_import>     ID of the migration group to create. Required.
 --prefix=<my_>                       String to prefix to the IDs of generated migrations.
 --post-type=<blog>                   Machine name of Drupal node bundle to hold imported post content.
 --post-text-format=<restricted_html> Machine name of text format for body field on imported post content.
 --page-type=<blog>                   Machine name of Drupal node bundle to hold imported page content.
 --page-text-format=<restricted_html> Machine name of text format for body field on imported page content.
 --category-vocabulary=<categories>   Machine name of vocabulary to hold imported categories.
 --tag-vocabulary=<tags>              Machine name of vocabulary to hold imported tags.
 --default-author=<author_account>    If present, username to author all imported content. If omitted, users will
                                      be imported from WordPress.
```

Thus, this command (on a Drupal 9 system where articles have a comment field but pages don't):

```
wordpress-migrate-generate /var/data/my_wp_export.xml --group-id=old_blog --prefix=blog_ --tag-vocabulary=tags --post-type=article --post-text-format=restricted_html --page-type=page --page-text-format=full_html
```

... will create the following migrations in the "old_blog" group:

- `blog_wordpress_authors`
- `blog_wordpress_categories`
- `blog_wordpress_tags`
- `blog_wordpress_content_post`
- `blog_wordpress_comment_post`
- `blog_wordpress_content_page`

You can then use Migrate Tools Drush commands like `drush mi --group=old_blog`
to manage the migrations.

### Support, known issues and plans

- Your support, questions and contributions are welcome.
  Please try to provide example files to help reproduce errors and notices:
  <https://drupal.org/project/issues/wordpress_migrate>
- Plan for 8.x-3.x beta release:
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

[WordPress Migrate SQL](https://www.drupal.org/project/wordpress_migrate_sql): Enables customized migrations based on WordPress SQL sites, allowing migration of complex WordPress sites, using a SQL source. [wp_migrate](https://www.drupal.org/project/wp_migrate) is another module which is compatible with up to Drupal 9.

## Credits

Current co-maintainer:

- [HongPong](https://drupal.org/u/HongPong)

Originally developed for Drupal 7 and 8 by [mikeryan](https://drupal.org/u/mikeryan).

Committers include:
somersoft, lomasr, chaitanya17, felribeiro, maccath, MaskyS,
mrmikedewolf, Darren Shelley, dwillems, othermachines, ohthehugemanatee,
ezeedub, grasmash, bdone, queenvictoria, ksenzee, ptaff, pverrier,
xurizaemon, hekele, aaron, emarchak, wizonesolutions, baltowen, msielski, ressa,
vlad.dancer, nitapawar, phjou, el7cosmos, batonac, stargayte, darchuletajr,
e.ruiter, ankshetty, i.vuchkov, sahana16081996, marktonino, john_b, frederickjh
