/* 
 * Cfgparser - configuration reader C++ library
 * Copyright (c) 2008, Seznam.cz, a.s.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Seznam.cz, a.s.
 * Radlicka 2, Praha 5, 15000, Czech Republic
 * http://www.seznam.cz, mailto:cfgparser@firma.seznam.cz
 * 
 * $Id: $
 *
 * PROJECT
 * Configuration file parser
 *
 * DESCRIPTION
 * Config parser test. Read test1.cfg and check results.
 *
 * AUTHOR
 * Solamyl <stepan@firma.seznam.cz>
 *
 */
 

#include <stdio.h>

#include "cfgparser.h"

using std::string;
using std::vector;


/*
 * Test returned config STRING value.
 * Returns: 0=ok, -1=error
 */
static int test_value(const ConfigParser_t &cfg, const string &sect,
        const string &name, const string &test_val)
{
    /* prepare default value */
    string value = "k423hjkhk56j24kvh2k34jbh234kjb5h2345jkbh2345kjh";
    if (cfg.getValue(sect, name, &value))
    {
        if (value != test_val)
        {
            printf("Error: getValue() returned unexpected value [%s] '%s' "
                    "= '%s'\n", sect.c_str(), name.c_str(), value.c_str());
            return -1; /*error*/
        }
    }
    else
    {
        printf("Error: [%s] '%s' entry not found\n",
                sect.c_str(), name.c_str());
        return -1; /*error*/
    }
    /* success */
    return 0;
}


/*
 * Test returned config INT value.
 * Returns: 0=ok, -1=error
 */
static int test_int_value(const ConfigParser_t &cfg, const string &sect,
        const string &name, int test_val)
{
    /* prepare default value */
    int value = -999999999;
    if (cfg.getValue(sect, name, &value))
    {
        if (value != test_val)
        {
            printf("Error: getValue() returned unexpected value [%s] '%s' "
                    "= %d\n", sect.c_str(), name.c_str(), value);
            return -1; /*error*/
        }
    }
    else
    {
        printf("Error: [%s] '%s' entry not found\n",
                sect.c_str(), name.c_str());
        return -1; /*error*/
    }
    /* success */
    return 0;
}


/*
 * Test returned config DOUBLE value.
 * Returns: 0=ok, -1=error
 */
static int test_double_value(const ConfigParser_t &cfg, const string &sect,
        const string &name, double test_val)
{
    /* prepare default value */
    double value = -999999999;
    if (cfg.getValue(sect, name, &value))
    {
        if (value != test_val)
        {
            printf("Error: getValue() returned unexpected value [%s] '%s' "
                    "= %f\n", sect.c_str(), name.c_str(), value);
            return -1; /*error*/
        }
    }
    else
    {
        printf("Error: [%s] '%s' entry not found\n",
                sect.c_str(), name.c_str());
        return -1; /*error*/
    }
    /* success */
    return 0;
}


/*
 * Test returned config MULTI value.
 * Returns: 0=ok, -1=error
 */
static int test_multi_value(const ConfigParser_t &cfg, const string &sect,
        const string &name, size_t count)
{
    /* prepare default value */
    vector<string> multi_value;
    if (cfg.getValue(sect, name, &multi_value))
    {
        if (multi_value.size() != count)
        {
            printf("Error: getMultiValue() returned unexpected count "
                    "of values [%s] 'count(%s)' == %Zd\n",
                    sect.c_str(), name.c_str(), multi_value.size());
            return -1; /*error*/
        }
    }
    else
    {
        printf("Error: [%s] '%s' entry not found\n",
                sect.c_str(), name.c_str());
        return -1; /*error*/
    }
    /* success */
    return 0;
}


/* main function */
int main(void)
{
    /* load sample config */
    ConfigParser_t cfg;
    if (cfg.readFile("test1.cfg"))
    {
        printf("Error: Cannot open config file 'test1.cfg'\n");
        return 1;
    }
}
