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

/*!
	\file main.cpp
	\brief Implementation of main()
*/

#include <QMainWindow>
#include <QApplication>

#include "calcgrid.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QStringList args = QCoreApplication::arguments();
	
	//CalcView view(args.count() > 1 ? args.at(1) : QString());
	//view.show();
	
	QMainWindow win;
	
	CalcGrid *g = new CalcGrid(&win);
	
	for ( int i = 1; i < args.count(); ++i )
		g->addCalc(args.at(i));
	
	win.setCentralWidget(g);
	
	win.show();
	
	return app.exec();
}
