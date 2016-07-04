/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

#ifndef	GCIF_H_INCLUDED
#define	GCIF_H_INCLUDED

#include <gtk/gtk.h>

typedef struct _LoanData LoanData;

#define	GCIF_VERSION_STRING	"3.12.19"

typedef enum
{
	FREQ_ANNUAL = 0,
	FREQ_SEMI_ANNUAL,
	FREQ_QUARTER,
	FREQ_MONTH,
	FREQ_DAY
} InterestFrequency;

struct _LoanData
{
	gdouble principle;
	gdouble rate;
	InterestFrequency freq;
	gint years;

	GtkWidget *output;
};
/* The main function */
void figure(LoanData *ld);


#endif	/* GCIF_H_INCLUDED */
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

#ifndef MENUS_H_INCLUDED
#define MENUS_H_INCLUDED

/* Creates a menubar */
GtkWidget *menubar_new(GtkWidget *window);

/* Callbacks */
/* '/Help/About' item */
void help_about_cb(void);

#endif /* MENUS_H_INCLUDED */
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED


/* Add commas to a passed string */
gchar *commifyS(gchar *string, gboolean free_old);
/* Add commas to the passed double, and put it into the passed string */
gchar *commifyD(gdouble value);
/* Add commas to the passed integer, and put it into the passed string */
gchar *commifyI(gint value);
/* Remove commas from a passed string */
gchar *decommifyS(gchar *string, gboolean free_old);


/* Sets the initial money value */
void set_principle(GtkWidget *entry, LoanData *ld);
/* Sets the interest rate */
void set_interest_rate(GtkWidget *entry, LoanData *ld);
/* Sets the term length */
void set_term_length(GtkWidget *entry, LoanData *ld);
/* Updates the figure's frequency if all of the items have been set */
void set_frequency(GtkComboBox *combo, LoanData *ld);

#endif	/* MISC_H_INCLUDED */
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

#include <string.h>
#include <math.h>

void figure(LoanData *ld)
{
	GString *output;
	gchar *principle_str, *rate_str, *result_str, *years_str, *gain_str;
	gdouble result;
	gint freq;

	switch(ld->freq)
	{
	    case FREQ_ANNUAL:
            freq = 1;
            break;
        case FREQ_SEMI_ANNUAL:
            freq = 2;
            break;
        case FREQ_QUARTER:
            freq = 4;
            break;
        case FREQ_MONTH:
            freq = 12;
            break;
        case FREQ_DAY:
            freq = 365;
            break;
		default:
			freq = 1;
			break;
	}

	/* Figure interest */
	result = ld->principle * (pow(1 + ((ld->rate / 100) / freq), ld->years * freq));

	/* Add commas to the initial amount and interest rate */
	principle_str = commifyD(ld->principle);
	rate_str = commifyD(ld->rate);

	/* Set this stuff in the message label area */
	output = g_string_new(NULL);
	g_string_append(output, g_strdup_printf("\tIf you started with $%s, with an interest rate of %s%%.\t\n", principle_str, rate_str));

	/* Add commas to the amount and years */
	result_str = commifyD(result);
	years_str = commifyI(ld->years);
	if(ld->years == 1)
		g_string_append(output, g_strdup_printf("\tAfter %s year, you will end up with $%s.\t\n", years_str, result_str));
	else
		g_string_append(output, g_strdup_printf("\tAfter %s years, you will end up with $%s.\t\n", years_str, result_str));

	/* Add commas to the 'result - principle' */
	gain_str = commifyD(result - ld->principle);
	g_string_append(output, g_strdup_printf("\tAlso, you gained $%s in interest.\t\n", gain_str));
	gtk_label_set_text(GTK_LABEL(ld->output), output->str);

	/* free stuff */
	g_string_free(output, TRUE);
	g_free(principle_str);
	g_free(rate_str);
	g_free(result_str);
	g_free(years_str);
	g_free(gain_str);
}
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/


int main(int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *vbox, *hbox;
	GtkWidget *menubar;
	GtkWidget *entry;
	GtkWidget *combo;
	GtkWidget *frame;

	LoanData ld;
	ld.principle = -1;
	ld.rate = -1;
	ld.years = -1;
	ld.output = NULL;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	gtk_window_set_title(GTK_WINDOW(window), "GCIF");

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	/* Menubar */
	menubar = menubar_new(window);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
	gtk_widget_show(menubar);

	/* Money amount frame */
	frame = gtk_frame_new("Principle");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	/* Entry to put in the initial amount of money */
	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 100);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(set_principle), &ld);
	gtk_container_add(GTK_CONTAINER(frame), entry);
	gtk_widget_show(entry);
	gtk_widget_set_tooltip_text(entry, "Enter your principle (starting amount of money) here");

	/* Interest rate frame */
	frame = gtk_frame_new("Interest Rate");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);

	/* Entry to put in the interest rate */
	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 100);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(set_interest_rate), &ld);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_widget_show(entry);
	gtk_widget_set_tooltip_text(entry, "Enter your interest rate here");

	/* Combo box for entering how often the interest is figured */
	combo = gtk_combo_box_new_text();
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(set_frequency), &ld);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Annually");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Bi-annually");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Quarterly");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Monthly");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Daily");
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);
	gtk_widget_set_tooltip_text(combo, "How often the interest rate is applied");

	/* Term length frame */
	frame = gtk_frame_new("Term Length (in years)");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	/* Entry to put in the term length */
	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 100);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(set_term_length), &ld);
	gtk_container_add(GTK_CONTAINER(frame), entry);
	gtk_widget_show(entry);
	gtk_widget_set_tooltip_text(entry, "Enter your term length here");

	/* Message frame */
	frame = gtk_frame_new("Message Area");
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show(frame);

	/* Message label */
	ld.output = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), ld.output);
	gtk_widget_show(ld.output);

	gtk_widget_show(window);

	gtk_main();
	return 0;
}
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

static const gchar *UI_string = {
	"<ui>\n"
	"	<menubar name=\"MainMenubar\">\n"
	"		<menu name=\"FileMenu\" action=\"FileMenuAction\">\n"
	"			<menuitem name=\"Quit\" action=\"QuitAction\"/>\n"
	"		</menu>\n"
	"		<menu name=\"HelpMenu\" action=\"HelpMenuAction\">\n"
	"			<menuitem name=\"About\" action=\"AboutAction\"/>\n"
	"		</menu>\n"
	"	</menubar>\n"
	"</ui>\n"
};

static GtkActionEntry action_entries[] = {
	{ "FileMenuAction", NULL, "_File" },
	{ "HelpMenuAction", NULL, "_Help" },

	{ "QuitAction", GTK_STOCK_QUIT, "_Quit", "<CTRL>Q", NULL, G_CALLBACK(gtk_main_quit) },
	{ "AboutAction", GTK_STOCK_ABOUT, "_About", "F1", NULL, G_CALLBACK(help_about_cb) },
};
static guint n_action_entries = G_N_ELEMENTS(action_entries);

static const gchar *license_text = {
	"This program is free software: you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation, either version 3 of the License, or\n"
	"(at your option) any later version.\n"
	"\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
	"GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License\n"
	"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
};

static const gchar *comments_text = {
	"GPL Cash Interest Figurer\n"
	"Copyright (C) 2007, 2008 by Tanner Jotblad\n"
};

GtkWidget *menubar_new(GtkWidget *parent)
{
	GtkActionGroup *action_group;
	GtkUIManager *uiman;
	GError *error;

	error = NULL;

	action_group = gtk_action_group_new("GcifActions");
	gtk_action_group_add_actions(action_group, action_entries, n_action_entries, NULL);

	uiman = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(uiman, action_group, 0);
	gtk_ui_manager_add_ui_from_string(uiman, UI_string, -1, &error);
	if(error != NULL)
	{
		g_warning("Could not create menus: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	gtk_window_add_accel_group(GTK_WINDOW(parent), gtk_ui_manager_get_accel_group(uiman));

	return gtk_ui_manager_get_widget(uiman, "/MainMenubar");
}

/* Callbacks */
void help_about_cb(void)
{
	GtkWidget *dlg;

	dlg = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), "GCIF");
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dlg), license_text);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), GCIF_VERSION_STRING);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg), comments_text);
	g_signal_connect(G_OBJECT(dlg), "response", G_CALLBACK(gtk_widget_destroy), (gpointer)dlg);

	gtk_widget_show(dlg);
}
/************************************************
*Begin GPL License Block
*
*Copyright (C) 2007  Tanner Jotblad <dreblenJ@gmail.com>
*
*This program is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*End GPL License Block
************************************************/

#include <string.h>
#include <stdlib.h>

gchar *commifyS(gchar *string, gboolean free_old)
{
	gchar *ret;
	gchar *post_decimal;
	gint len, newlen, newleftlen, leftlen, rightlen;
	gint i, j;

	g_return_val_if_fail(string != NULL, NULL);

	len = strlen(string);

	post_decimal = g_strrstr(string, ".");
	if(post_decimal == NULL)
	{
		post_decimal = const_cast<char *>("0");
		leftlen = len;
	}
	else
	{
		leftlen = post_decimal - string;
		post_decimal++;
	}
	rightlen = strlen(post_decimal);

	/* leftlen + commas */
	newleftlen = leftlen + (leftlen/3);
	/* minus one if divisible by three */
	if(len%3 == 0)
		newleftlen--;
	/* newleftlen + rightlen + '.' + '\0' */
	newlen = newleftlen + rightlen + 2;

	ret = (gchar*)g_malloc(newlen);
	for(i = 0, j = 0; i < leftlen; i++, j++)
	{
		if(i%3 == 0 && i != 0)
		{
			ret[j++] = ',';
			ret[j] = string[leftlen-i-1];
		}
		else
			ret[j] = string[leftlen-i-1];
	}
	ret[newleftlen] = '\0';
	g_strreverse(ret);

	strcat(ret, ".");
	strcat(ret, post_decimal);

	if(free_old == TRUE)
		g_free(string);

	return ret;
}

gchar *commifyD(gdouble value)
{
	gchar *ret;

	ret = g_strdup_printf("%.2f", value);
	return commifyS(ret, TRUE);
}

gchar *commifyI(gint value)
{
	gchar *ret;

	ret = g_strdup_printf("%d", value);
	return commifyS(ret, TRUE);
}

gchar *decommifyS(gchar *string, gboolean free_old)
{
	gchar *ret;
	gint i, j;

	ret = (gchar*)g_malloc(strlen(string) + 1);

	for(i = j = 0; i < strlen(string); i++)
	{
		if(string[i] != ',')
			ret[j++] = string[i];
	}
	ret[j] = '\0';

	if(free_old == TRUE)
		g_free(string);

	return ret;
}

void set_principle(GtkWidget *entry, LoanData *ld)
{
	const gchar *str;
	gchar *temp;

	str = gtk_entry_get_text(GTK_ENTRY(entry));
	temp = decommifyS((gchar*)str, FALSE);
	ld->principle = atof(temp);
	g_free(temp);
	if(ld->principle != -1 && ld->rate != -1 && ld->years != -1)
		figure(ld);
}

void set_interest_rate(GtkWidget *entry, LoanData *ld)
{
	const gchar *str;
	gchar *temp;

	str = gtk_entry_get_text(GTK_ENTRY(entry));
	temp = decommifyS((gchar*)str, FALSE);
	ld->rate = atof(temp);
	if(ld->principle != -1 && ld->rate != -1 && ld->years != -1)
		figure(ld);
}

void set_term_length(GtkWidget *entry, LoanData *ld)
{
	const gchar *str;
	gchar *temp;

	str = gtk_entry_get_text(GTK_ENTRY(entry));
	temp = decommifyS((gchar*)str, FALSE);
	ld->years = atoi(temp);
	if(ld->principle != -1 && ld->rate != -1 && ld->years != -1)
		figure(ld);
}

void set_frequency(GtkComboBox *combo, LoanData *ld)
{
	ld->freq = static_cast<InterestFrequency>(gtk_combo_box_get_active(combo));
    if(ld->principle != -1 && ld->rate != -1 && ld->years != -1)
		figure(ld);
}
