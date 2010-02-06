#ifndef __PKG_VERSION_H__
#define __PKG_VERSION_H__

/**
 * SECTION:serenity-version
 * @title: Versioning API
 * @short_description: compile-time version checking
 *
 * provides some API and definitions for compile-time version checking.
 */

/**
 * PKG_MAJOR_VERSION:
 *
 * Major version of Serenity, e.g. 1 in "1.2.3"
 */
#define PKG_MAJOR_VERSION (0)

/**
 * PKG_MINOR_VERSION:
 *
 * Minor version of Serenity, e.g. 2 in "1.2.3"
 */
#define PKG_MINOR_VERSION (1)

/**
 * PKG_MICRO_VERSION:
 *
 * Micro version of Serenity, e.g. 3 in "1.2.3"
 */
#define PKG_MICRO_VERSION (1)

/**
 * PKG_API_VERSION_S:
 *
 * Version of the API of Serenity
 */
#define PKG_API_VERSION_S "1.0"

/**
 * PKG_VERSION_S:
 *
 * Stringified version of Serenity, e.g. "1.2.3".
 *
 * Useful for display.
 */
#define PKG_VERSION_S ""

/**
 * PKG_VERSION_HEX:
 *
 * Hexadecimally encoded version of Serenity, e.g. 0x01020300"
 *
 * Useful for comparisons.
 */
#define PKG_VERSION_HEX (PKG_MAJOR_VERSION << 24 | PKG_MINOR_VERSION << 16 | PKG_MICRO_VERSION << 8)

/**
 * PKG_CHECK_VERSION:
 * @major: major component of the version to check
 * @minor: minor component of the version to check
 * @micro: micro component of the version to check
 *
 * Checks whether the decomposed version (@major, @minor, @micro) is
 * bigger than the version of Serenity. This is a compile-time
 * check only.
 */
#define PKG_CHECK_VERSION(major,minor,micro)   \
        (PKG_MAJOR_VERSION >= (major) ||       \
         (PKG_MAJOR_VERSION == (major) &&      \
          PKG_MINOR_VERSION > (minor)) ||      \
         (PKG_MAJOR_VERSION == (major) &&      \
          PKG_MINOR_VERSION == (minor) &&      \
          PKG_MICRO_VERSION >= (micro)))

#endif /* __PKG_VERSION_H__ */
