/****************************************************************************
 **
 ** Copyright (c) 2015 Digia Plc
 ** For any questions to Digia, please use contact form at http://qt.digia.com/
 **
 ** All Rights Reserved.
 **
 ** NOTICE: All information contained herein is, and remains
 ** the property of Digia Plc and its suppliers,
 ** if any. The intellectual and technical concepts contained
 ** herein are proprietary to Digia Plc
 ** and its suppliers and may be covered by Finnish and Foreign Patents,
 ** patents in process, and are protected by trade secret or copyright law.
 ** Dissemination of this information or reproduction of this material
 ** is strictly forbidden unless prior written permission is obtained
 ** from Digia Plc.
 ****************************************************************************/
#ifndef SCXMLGLOBALS_H
#define SCXMLGLOBALS_H
#include <qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  ifdef QT_BUILD_SCXML_LIB
#    define Q_SCXML_EXPORT Q_DECL_EXPORT
#  else
#    define Q_SCXML_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_SCXML_EXPORT
#endif

QT_END_NAMESPACE

#endif // SCXMLGLOBALS_H

