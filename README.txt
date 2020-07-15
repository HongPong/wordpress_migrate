WordPress Migrate module for Drupal 8.x

The WordPress Migrate module provides tools for setting up migration processes
from the WordPress blog to a Drupal 8 site. By providing a few configuration
settings and a pointer to an XML export file, migration configuration entities
will be generated which can then be executed or otherwise managed with the
migrate_tools module.

This module has been developed since 2010 to reliably import WordPress sites into
Drupal.

There are a few ways to make use of wordpress_migrate:

Drush command
=============
A single drush command, wordpress-migrate-generate, is provided for generating
WordPress migrations from a few simple options:

Arguments:
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

Thus, this command (on a D8 system where articles have a comment field but pages don't):

wordpress-migrate-generate /var/data/my_wp_export.xml --group-id=old_blog --prefix=blog_ --tag-vocabulary=tags --post-type=article --post-text-format=restricted_html --page-type=page --page-text-format=full_html

will create the following migrations in the "old_blog" group:

 blog_wordpress_authors
 blog_wordpress_categories
 blog_wordpress_tags
 blog_wordpress_content_post
 blog_wordpress_comment_post
 blog_wordpress_content_page

You can then use migrate_tools drush commands like "drush mi --group=old_blog"
to manage the migrations.

This command is not yet Drush 9 compatible. See issue:
https://www.drupal.org/project/wordpress_migrate/issues/2955644

UI
==
Enabling the wordpress_migrate_ui module adds an "Add import from WordPress"
button to the migrate_tools UI at /admin/structure/migrate - this begins a
wizard which prompts you for the same configuration options you see for the
drush command above.

Importing image assets
======================

For the new importation of attached images including post thumbnails
(also known as featured images) you should:
 - Assign an image field for these assets in the migrate process.
 - Before importing the content posts, import the media assets.
 - Import the media assets.

See issue for more information and customization:
https://www.drupal.org/project/wordpress_migrate/issues/2742269

Important: While image assets can be imported to the Drupal filesystem,
the paths of inline images from the WordPress body are not rewritten.
See issue: https://www.drupal.org/project/wordpress_migrate/issues/2742279

API
===
You may also programmatically configure a set of WordPress migrations by
constructing a configuration array and passing it to the generator:

  use Drupal\wordpress_migrate\WordPressMigrationGenerator;

  $configuration = [
   'file_uri' => '/var/data/my_wp_export.xml',
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

Documentation
=============
A new documentation page is being developed here:
https://www.drupal.org/docs/8/modules/wordpress-migrate

The Drupal 7 documentation is available here and partly relevant:
https://www.drupal.org/node/1593370

Support, known issues and plans
===============================
Your support, questions and contributions are welcome:
https://www.drupal.org/project/issues/wordpress_migrate
Please try to provide example files to help reproduce errors and notices.

Plan for 8.x-3.x beta release:
https://www.drupal.org/project/wordpress_migrate/issues/2904990

Comment migration may need to set a body text format:
https://www.drupal.org/project/wordpress_migrate/issues/2742311

Drush 9 support:
https://www.drupal.org/project/wordpress_migrate/issues/2955644

Random strings in taxonomies:
https://www.drupal.org/project/wordpress_migrate/issues/2974024

Permalinks and URL alias tables:
https://www.drupal.org/project/wordpress_migrate/issues/2869595
https://www.drupal.org/project/wordpress_migrate/issues/2904545

Rewrite local link/image references in content:
https://www.drupal.org/project/wordpress_migrate/issues/2742279

Extract and save blog metadata:
https://www.drupal.org/project/wordpress_migrate/issues/2742287

Credits
=======

Current co-maintainer:
* HongPong ( https://drupal.org/u/HongPong )

Originally developed for Drupal 7 and 8 by mikeryan.

Committers include:
somersoft, lomasr, chaitanya17, felribeiro, maccath, MaskyS,
mrmikedewolf, Darren Shelley, dwillems, othermachines, ohthehugemanatee,
ezeedub, grasmash, bdone, queenvictoria, ksenzee, ptaff, pverrier,
xurizaemon, hekele, aaron, emarchak, wizonesolutions
