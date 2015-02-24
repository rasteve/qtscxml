/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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
#ifdef SCXML_LIBRARY
#define SCXML_EXPORT Q_DECL_EXPORT
#else
#define SCXML_EXPORT
#endif
#ifdef QT_DEBUG
#define SCXML_DEBUG
#endif
#endif // SCXMLGLOBALS_H

