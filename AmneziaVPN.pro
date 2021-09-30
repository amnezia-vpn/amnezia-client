TEMPLATE = subdirs
SUBDIRS = client

!ios:!android {
   SUBDIRS += service platform
}
