/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is xTuple ERP: PostBooks Edition 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by xTuple ERP: PostBooks Edition
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "copySalesOrder.h"

#include <QSqlError>
#include <QVariant>

#include "inputManager.h"
#include "salesOrderList.h"

copySalesOrder::copySalesOrder(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_so, SIGNAL(newId(int)), this, SLOT(sPopulateSoInfo(int)));
  connect(_soList, SIGNAL(clicked()), this, SLOT(sSoList()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_copy, SIGNAL(clicked()), this, SLOT(sCopy()));
  connect(_so, SIGNAL(requestList()), this, SLOT(sSoList()));

  _captive = FALSE;

#ifndef Q_WS_MAC
  _soList->setMaximumWidth(25);
#endif

  omfgThis->inputManager()->notify(cBCSalesOrder, this, _so, SLOT(setId(int)));

  _soitem->addColumn(tr("#"),        _seqColumn, Qt::AlignRight, true, "coitem_linenumber" );
  _soitem->addColumn(tr("Item"),    _itemColumn, Qt::AlignLeft,  true, "item_number");
  _soitem->addColumn(tr("Description"),      -1, Qt::AlignLeft,  true, "item_descrip");
  _soitem->addColumn(tr("Site"),     _whsColumn, Qt::AlignCenter,true, "warehous_code");
  _soitem->addColumn(tr("Ordered"),  _qtyColumn, Qt::AlignRight, true, "coitem_qtyord");
  _soitem->addColumn(tr("Price"),    _qtyColumn, Qt::AlignRight, true, "coitem_price");
  _soitem->addColumn(tr("Extended"), _qtyColumn, Qt::AlignRight, true, "extended");
}

copySalesOrder::~copySalesOrder()
{
  // no need to delete child widgets, Qt does it all for us
}

void copySalesOrder::languageChange()
{
  retranslateUi(this);
}

enum SetResponse copySalesOrder::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("sohead_id", &valid);
  if (valid)
  {
    _captive = TRUE;
    _so->setId(param.toInt());
    _so->setEnabled(FALSE);
    _soList->setEnabled(FALSE);

    _copy->setFocus();
  }

  return NoError;
}


void copySalesOrder::sSoList()
{
  ParameterList params;
  params.append("sohead_id", _so->id());
  params.append("soType", cSoOpen);
  
  salesOrderList newdlg(this, "", TRUE);
  newdlg.set(params);

  _so->setId(newdlg.exec());
}


void copySalesOrder::sPopulateSoInfo(int)
{
  if (_so->id() != -1)
  {
    q.prepare( "SELECT cohead_number,"
              "        cohead_orderdate,"
              "        cohead_custponumber, cust_name, cust_phone "
              "FROM cohead, cust "
              "WHERE ( (cohead_cust_id=cust_id)"
              " AND (cohead_id=:sohead_id) );" );
    q.bindValue(":sohead_id", _so->id());
    q.exec();
    if (q.first())
    {
      _orderDate->setDate(q.value("cohead_orderdate").toDate());
      _poNumber->setText(q.value("cohead_custponumber").toString());
      _custName->setText(q.value("cust_name").toString());
      _custPhone->setText(q.value("cust_phone").toString());
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "SELECT coitem.*, item_number,"
	       "       (item_descrip1 || ' ' || item_descrip2) AS item_descrip,"
	       "       warehous_code,"
               "       ((coitem_qtyord * coitem_qty_invuomratio) * (coitem_price / coitem_price_invuomratio)) AS extended,"
               "       'qty' AS coitem_qtyord_xtnumericrole,"
               "       'price' AS coitem_price_xtnumericrole,"
               "       'curr' AS extended_xtnumericrole "
               "FROM coitem, itemsite, item, warehous "
               "WHERE ( (coitem_itemsite_id=itemsite_id)"
               " AND (itemsite_item_id=item_id)"
               " AND (itemsite_warehous_id=warehous_id)"
               " AND (coitem_status != 'X')"
               " AND (coitem_cohead_id=:sohead_id) "
               " AND (coitem_subnumber = 0) ) "
               "ORDER BY coitem_linenumber;" );
    q.bindValue(":sohead_id", _so->id());
    q.exec();
    _soitem->populate(q);
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _orderDate->clear();
    _poNumber->clear();
    _custName->clear();
    _custPhone->clear();
    _soitem->clear();
  }
}


void copySalesOrder::sCopy()
{
  q.prepare("SELECT copySo(:sohead_id, :scheddate) AS sohead_id;");
  q.bindValue(":sohead_id", _so->id());

  if (_reschedule->isChecked())
    q.bindValue(":scheddate", _scheduleDate->date());

  q.exec();

  if (_captive)
  {
    if (q.first())
    {
      int soheadid = q.value("sohead_id").toInt();
      omfgThis->sSalesOrdersUpdated(soheadid);
      done(soheadid);
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _so->setId(-1);
    _so->setFocus();
  }
}
