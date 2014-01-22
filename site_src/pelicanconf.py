#!/usr/bin/env python
# -*- coding: utf-8 -*- #
from __future__ import unicode_literals

AUTHOR = u'Will Ouyang'
SITENAME = u'EVA'
SITEURL = ''

TIMEZONE = 'Asia/Shanghai'

DEFAULT_LANG = u'en'

# Feed generation is usually not desired when developing
FEED_ALL_ATOM = None
CATEGORY_FEED_ATOM = None
TRANSLATION_FEED_ATOM = None

THEME = 'bootstrapTheme4eva'

DISQUS_SITENAME = 'evaimg'
TWITTER_USERNAME = 'oeway'

NDE_CATEGORIES = (
            
            ('Bioimaging','pages/bioimaging.html'),
            ('-','-'),
            ('Ultrasonic','pages/ultrasonic.html'),
            ('Radiography', 'pages/radiography.html'),
            ('Eddy-current', 'pages/eddycurrent.html'),
            ('-','-'),
            
            
            )
MENUITEMS =(('Home',''),
            ('Blog','blog.html'),
            ('Wiki','https://github.com/oeway/EVA/wiki'),
            #('Categories_dropdown', NDE_CATEGORIES ),
            ('Downloads','pages/downloads.html'),
            ('About','pages/about.html'),
            )

CAROUSELITEMS = (
            {
            'image':'images/slide-01.png',  
            'headline':'Imaging and Evaluation',
            'subtitle':'An Open-souce Platform',
            'buttonLink':'pages/about.html',
            'buttonCaption':'Learn More',
            },
            
)
# static paths will be copied without parsing their contents
FILE_TO_COPY = [
                'extra/robots.txt',
                ]
EXTRA_PATH_METADATA = {
    'extra/robots.txt': {'path': 'robots.txt'},
}

DIRECT_TEMPLATES = ('index', 'tags', 'categories', 'archives','blog','editor')
PLUGINS_SAVE_AS = 'blog.html'

USE_FOLDER_AS_CATEGORY = True
ARTICLE_URL = '{category}/{slug}.html'
ARTICLE_SAVE_AS = '{category}/{slug}.html'


# Blogroll
LINKS =  (('Pelican', 'http://getpelican.com/'),
          ('Python.org', 'http://python.org/'),
          ('Jinja2', 'http://jinja.pocoo.org/'),
         
         )

# Social widget
SOCIAL = (('You can add links in your config file', '#'),
          ('Another social link', '#'),)

DEFAULT_PAGINATION = 10

# Uncomment following line if you want document-relative URLs when developing
#RELATIVE_URLS = True


