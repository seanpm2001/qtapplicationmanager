TEMPLATE = subdirs

SUBDIRS = \
    hello-world \
    minidesk \
    monitor \
    multi-views \
    startup-plugin \

# remove the !headless and handle this in the example when we switch to the new configure system
!headless:SUBDIRS += \
    custom-appman \

linux:!android:SUBDIRS += \
    softwarecontainer-plugin \
