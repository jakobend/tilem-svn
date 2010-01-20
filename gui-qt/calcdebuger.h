/****************************************************************************
**
** Copyright (C) 2009 Hugues Luc BRUANT aka fullmetalcoder 
**                    <non.deterministic.finite.organism@gmail.com>
** 
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef _CALC_DEBUGER_H_
#define _CALC_DEBUGER_H_

/*!
	\file calcdebuger.h
	\brief Definition of the CalcDebuger class
*/

#include <QWidget>
#include "ui_calcdebuger.h"

extern "C" {
#include <tilemdb.h>
}

#include <QPointer>

class Calc;
class CalcGrid;

class CalcDebuger : public QWidget, private Ui::CalcDebuger
{
	Q_OBJECT
	
	public:
		CalcDebuger(CalcGrid *g, QWidget *p = 0);
		~CalcDebuger();
		
		virtual QSize sizeHint() const;
		
	protected:
		virtual void timerEvent(QTimerEvent *e);
		
	private slots:
		void on_cb_target_currentIndexChanged(int idx);
		
		void on_lw_breakpoints_customContextMenuRequested(const QPoint& p);
		void currentBreakpointChanged(const QModelIndex& idx);
		
		void on_cb_break_type_currentIndexChanged(int idx);
		
		void on_rb_break_physical_toggled(bool y);
		
		void on_le_break_start_addr_textEdited(const QString& s);
		void on_le_break_end_addr_textEdited(const QString& s);
		void on_le_break_mask_addr_textEdited(const QString& s);
		
		void on_spn_refresh_valueChanged(int val);
		
		void on_le_disasm_start_textChanged(const QString& s);
		void on_spn_disasm_length_valueChanged(int val);
		
		void breakpoint(int id);
		
	private:
		void updateDisasm(dword addr, int len);
		
		int m_refreshId;
		
		CalcGrid *m_calcGrid;
		QPointer<Calc> m_calc;
		
		TilemDisasm *m_disasm;
};

#endif
