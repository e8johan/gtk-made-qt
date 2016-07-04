/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
  * 
 * gqt, implementing the GTK APIs through Qt
 * Copyright (C) Robin Burchell
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Lesser GNU General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

QWidget *gtk_table_new(int, int, bool)
{
    QWidget *w = new QWidget;
    w->setLayout(new QGridLayout);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

void gtk_table_attach_defaults(QObject *o_p, QObject *o_cw, int l, int r, int t, int b)
{
    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(cw)

    QGridLayout *gl = qobject_cast<QGridLayout*>(p->layout());
    Q_ASSERT(gl);
    gl->addWidget(cw, t, l, b-t, r-l);
}

// XXX: implement homogenous
// NOTE: columns and rows are ignored here, Qt doesn't require them
void gtk_table_attach(GtkTable *table, GtkWidget *child,
		guint left_attach, guint right_attach,
		guint top_attach, guint bottom_attach,
		int xopt, int yopt,
		guint xpadding, guint ypadding)
{
	Q_ASSERT(table && table->layout() && child);

	// Make sure the column/rowspan values are going to be kosher
	Q_ASSERT(right_attach - left_attach > 0);
	Q_ASSERT(bottom_attach - top_attach > 0);

	qDebug("Adding item to table, left: %d, top %d, width %d, height %d", left_attach, top_attach, right_attach - left_attach, bottom_attach - top_attach);

	QGridLayout *g = dynamic_cast<QGridLayout *>(table->layout());
	Q_ASSERT(g);

	g->addWidget(child, top_attach, left_attach, right_attach - left_attach, bottom_attach - top_attach, Qt::AlignCenter);
	child->setParent(table);
}
