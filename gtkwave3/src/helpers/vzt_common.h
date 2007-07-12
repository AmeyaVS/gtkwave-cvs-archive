/*
 * Copyright (c) 2004-2007 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef DEFS_VZTC_H
#define DEFS_VZTC_H

#ifndef _MSC_VER
typedef uint8_t                 vztint8_t;
typedef uint16_t                vztint16_t;
typedef uint32_t                vztint32_t;
typedef uint64_t                vztint64_t;

typedef int32_t                 vztsint32_t;
typedef uint64_t 		vzttime_t;

#else
typedef unsigned __int8         vztint8_t;
typedef unsigned __int16        vztint16_t;
typedef unsigned __int32        vztint32_t;
typedef unsigned __int64        vztint64_t;

typedef          __int32        vztsint32_t;
typedef unsigned __int64        vzttime_t;
#endif

#endif