/*==============================================================================
File    conf_parser.hpp

Author  Daniel Zorychta

Brief   Configuration parser.

        Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

==============================================================================*/

/**
@defgroup CONF_PARSER_HPP_ CONF_PARSER_HPP_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include "conf_parser.h"

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
class conf_parser {
public:
        conf_parser(void)
        {
                this->parser = NULL;
        }

        ~conf_parser(void)
        {
                conf_parser__delete(this->parser);
        }

        int set_file_path(const char *path)
        {
                return conf_parser__set_file_path(this->parser, path);
        }

        int load_file(const char *path)
        {
                if (not this->parser) {
                        this->parser = conf_parser__new(NULL);
                }

                return conf_parser__load_file(this->parser, path);
        }

        int create_empty(void)
        {
                if (not this->parser) {
                        this->parser = conf_parser__new(NULL);
                }

                return this->parser ? conf_parser_status__success
                                    : conf_parser_status__no_memory;
        }

        int create_empty(const char *path)
        {
                if (not this->parser) {
                        this->parser = conf_parser__new(NULL);
                }

                int err = this->set_file_path(path);
                if (err) {
                        return err;
                }

                return this->parser ? conf_parser_status__success
                                    : conf_parser_status__no_memory;
        }

        int load_buffer(const char *buf, size_t bufsz)
        {
                if (not this->parser) {
                        this->parser = conf_parser__new(NULL);
                }

                return conf_parser__load_buffer(this->parser, buf, bufsz);
        }

        int save(void)
        {
                return conf_parser__save(this->parser);
        }

        int reload(void)
        {
                return conf_parser__reload(this->parser);
        }

        int save_file(const char *path)
        {
                return conf_parser__save_file(this->parser, path);
        }

        int save_buffer(char *buf, size_t bufsz)
        {
                return conf_parser__save_buffer(this->parser, buf, bufsz);
        }

        const char *get_value(const char *key)
        {
                return conf_parser__get_value(this->parser, key);
        }

        char *get_file_value(const char *path, const char *key)
        {
                return conf_parser__get_file_value(path, key);
        }

        int set_value(const char *key, const char *value)
        {
                return conf_parser__set_value(this->parser, key, value);
        }

        int set_file_value(const char *path, const char *key, const char *value)
        {
                return conf_parser__set_file_value(path, key, value);
        }

        int get_pair(size_t i, const char **key, const char **val)
        {
                return conf_parser__get_pair(this->parser, i, key, val);
        }

        static bool is_value_true(const char *value)
        {
                return conf_parser__is_value_true(value);
        }

private:
        conf_parser_t *parser;
};

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
