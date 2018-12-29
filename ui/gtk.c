/*
    mtr  --  a network diagnostic tool
    Copyright (C) 1997,1998  Matt Kimball
    Changes/additions Copyright (C) 1998 R.E.Wolff@BitWizard.nl

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef HAVE_GTK
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include "mtr.h"
#include "net.h"
#include "dns.h"
#include "asn.h"
#include "mtr-gtk.h"
#include "utils.h"

#include "img/mtr_icon.xpm"

#include <iconv.h>
#include <errno.h>
#define QQWRY "QQWry.dat"
#define REDIRECT_MODE_1 0x01
#define REDIRECT_MODE_2 0x02
#define MAXBUF 255

#endif

static gint gtk_ping(
    gpointer data);
static gint Copy_activate(
    GtkWidget * widget,
    gpointer data);
static gint NewDestination_activate(
    GtkWidget * widget,
    gpointer data);
static gboolean ReportTreeView_clicked(
    GtkWidget * Tree,
    GdkEventButton * event,
    gpointer data);
static gchar *getSelectedHost(
    GtkTreePath * path);

static int ping_timeout_timer;
static GtkWidget *Pause_Button;
static GtkWidget *Entry;
static GtkWidget *main_window;

static void gtk_add_ping_timeout(
    struct mtr_ctl *ctl)
{
    int dt;

    if (gtk_toggle_button_get_active((GtkToggleButton *) Pause_Button)) {
        return;
    }
    dt = calc_deltatime(ctl->WaitTime);
    gtk_redraw(ctl);
    ping_timeout_timer = g_timeout_add(dt / 1000, gtk_ping, ctl);
}


static void gtk_do_init(
    int *argc,
    char ***argv)
{
    static int done = 0;

    if (!done) {
        gtk_init(argc, argv);

        done = 1;
    }
}


int gtk_detect(
    ATTRIBUTE_UNUSED int *argc,
    ATTRIBUTE_UNUSED char ***argv)
{
    if (getenv("DISPLAY") != NULL) {
        /* If we do this here, gtk_init exits on an error. This happens
           BEFORE the user has had a chance to tell us not to use the 
           display... */
        return TRUE;
    } else {
        return FALSE;
    }
}


static gint Window_destroy(
    ATTRIBUTE_UNUSED GtkWidget * Window,
    ATTRIBUTE_UNUSED gpointer data)
{
    gtk_main_quit();

    return FALSE;
}


static gint Restart_clicked(
    ATTRIBUTE_UNUSED GtkWidget * Button,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    net_reset(ctl);
    gtk_redraw(ctl);

    return FALSE;
}


static gint Pause_clicked(
    ATTRIBUTE_UNUSED GtkWidget * Button,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    static int paused = 0;

    if (paused) {
        gtk_add_ping_timeout(ctl);
    } else {
        g_source_remove(ping_timeout_timer);
    }
    paused = !paused;
    gtk_redraw(ctl);

    return FALSE;
}

static gint About_clicked(
    ATTRIBUTE_UNUSED GtkWidget * Button,
    ATTRIBUTE_UNUSED gpointer data)
{
    static const gchar *authors[] = {
        "Matt Kimball <matt.kimball@gmail.com>",
        "Roger Wolff <R.E.Wolff@BitWizard.nl>",
        "Bohdan Vlasyuk <bohdan@cec.vstu.vinnica.ua>",
        "Evgeniy Tretyak <evtr@ukr.net>",
        "John Thacker <thacker@math.cornell.edu>",
        "Juha Takala",
        "David Sward <sward@clark.net>",
        "David Stone <stone@AsIf.com>",
        "Andrew Stesin",
        "Greg Stark <gsstark@mit.edu>",
        "Robert Sparks <rjsparks@nostrum.com>",
        "Mike Simons <msimons@moria.simons-clan.com>",
        "Aaron Scarisbrick,",
        "Craig Milo Rogers <Rogers@ISI.EDU>",
        "Antonio Querubin <tony@lavanauts.org>",
        "Russell Nelson <rn-mtr@crynwr.com>",
        "Davin Milun <milun@acm.org>",
        "Josh Martin <jmartin@columbiaservices.net>",
        "Alexander V. Lukyanov <lav@yars.free.net>",
        "Charles Levert <charles@comm.polymtl.ca> ",
        "Bertrand Leconte <B.Leconte@mail.dotcom.fr>",
        "Anand Kumria",
        "Olav Kvittem <Olav.Kvittem@uninett.no>",
        "Adam Kramer <l3zqc@qcunix1.acc.qc.edu> ",
        "Philip Kizer <pckizer@nostrum.com>",
        "Simon Kirby",
        "Sami Kerola <kerolasa@iki.fi>",
        "Christophe Kalt",
        "Steve Kann <stevek@spheara.horizonlive.com>",
        "Brett Johnson <brett@jdacareers.com>",
        "Roland Illig <roland.illig@gmx.de>",
        "Damian Gryski <dgryski@uwaterloo.ca>",
        "Rob Foehl <rwf@loonybin.net>",
        "Mircea Damian",
        "Cougar <cougar@random.ee>",
        "Travis Cross <tc@traviscross.com>",
        "Brian Casey",
        "Andrew Brown <atatat@atatdot.net>",
        "Bill Bogstad <bogstad@pobox.com> ",
        "Marc Bejarano <marc.bejarano@openwave.com>",
        "Moritz Barsnick <barsnick@gmx.net>",
        "Thomas Klausner <wiz@NetBSD.org>",
        NULL
    };

    gtk_show_about_dialog(GTK_WINDOW(main_window)
                          , "version", PACKAGE_VERSION, "copyright",
                          "Copyright \xc2\xa9 1997,1998  Matt Kimball",
                          "website", "http://www.bitwizard.nl/mtr/",
                          "authors", authors, "comments",
                          "The 'traceroute' and 'ping' programs in a single network diagnostic tool.",
                          "license",
                          "This program is free software; you can redistribute it and/or modify\n"
                          "it under the terms of the GNU General Public License version 2 as\n"
                          "published by the Free Software Foundation.\n"
                          "\n"
                          "This program is distributed in the hope that it will be useful,\n"
                          "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                          "GNU General Public License for more details.",
                          NULL);
    return TRUE;
}

/*
 * There is a small problem with the following code:
 * The timeout is canceled and removed in order to ensure that
 * it takes effect (consider what happens if you set the timeout to 999,
 * then try to undo the change); is a better approach possible?
 *
 * What's the problem with this? (-> "I don't think so)
 */

static gint WaitTime_changed(
    ATTRIBUTE_UNUSED GtkAdjustment * Adj,
    GtkWidget * data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;
    GtkWidget *Button = (GtkWidget *) ctl->gtk_data;

    ctl->WaitTime = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Button));
    g_source_remove(ping_timeout_timer);
    gtk_add_ping_timeout(ctl);
    gtk_redraw(ctl);

    return FALSE;
}


static gint Host_activate(
    GtkWidget * entry,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;
    struct hostent *addr;

    addr = dns_forward(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (addr) {
        net_reopen(ctl, addr);
        /* If we are "Paused" at this point it is usually because someone
           entered a non-existing host. Therefore do the go-ahead... */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Pause_Button), 0);
    } else {
        int pos = strlen(gtk_entry_get_text(GTK_ENTRY(entry)));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Pause_Button), 1);
        gtk_editable_insert_text(GTK_EDITABLE(entry), ": not found", -1,
                                 &pos);
    }

    return FALSE;
}



static void Toolbar_fill(
    struct mtr_ctl *ctl,
    GtkWidget * Toolbar)
{
    GtkWidget *Button;
    GtkWidget *Label;
    GtkAdjustment *Adjustment;

    Button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_box_pack_end(GTK_BOX(Toolbar), Button, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(Button), "clicked",
                     GTK_SIGNAL_FUNC(Window_destroy), NULL);

    Button = gtk_button_new_from_stock(GTK_STOCK_ABOUT);
    gtk_box_pack_end(GTK_BOX(Toolbar), Button, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(Button), "clicked",
                     GTK_SIGNAL_FUNC(About_clicked), NULL);

    Button = gtk_button_new_with_mnemonic("_Restart");
    gtk_box_pack_end(GTK_BOX(Toolbar), Button, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(Button), "clicked",
                     GTK_SIGNAL_FUNC(Restart_clicked), ctl);

    Pause_Button = gtk_toggle_button_new_with_mnemonic("_Pause");
    gtk_box_pack_end(GTK_BOX(Toolbar), Pause_Button, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(Pause_Button), "clicked",
                     GTK_SIGNAL_FUNC(Pause_clicked), ctl);

    /* allow root only to set zero delay */
    Adjustment = (GtkAdjustment *) gtk_adjustment_new(ctl->WaitTime,
                                                      running_as_root() ? 0.01 : 1.00,
                                                      999.99, 1.0, 10.0,
                                                      0.0);
    Button = gtk_spin_button_new(Adjustment, 0.5, 2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(Button), TRUE);
    gtk_box_pack_end(GTK_BOX(Toolbar), Button, FALSE, FALSE, 0);
    ctl->gtk_data = Button;
    g_signal_connect(GTK_OBJECT(Adjustment), "value_changed",
                     GTK_SIGNAL_FUNC(WaitTime_changed), ctl);

    Label = gtk_label_new_with_mnemonic("_Hostname:");
    gtk_box_pack_start(GTK_BOX(Toolbar), Label, FALSE, FALSE, 0);

    Entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(Entry), ctl->Hostname);
    g_signal_connect(GTK_OBJECT(Entry), "activate",
                     GTK_SIGNAL_FUNC(Host_activate), ctl);
    gtk_box_pack_start(GTK_BOX(Toolbar), Entry, TRUE, TRUE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(Label), Entry);
}

static GtkWidget *ReportTreeView;
static GtkListStore *ReportStore;

enum {
#ifdef HAVE_IPINFO
    COL_ASN,
#endif
    COL_HOSTNAME,
    COL_ADDRESS,
    COL_LOSS,
    COL_RCV,
    COL_SNT,
    COL_LAST,
    COL_BEST,
    COL_AVG,
    COL_WORST,
    COL_STDEV,
    COL_COLOR,
    N_COLS
};

/* Trick to cast a pointer to integer.  We are mis-using a pointer as a
   single integer.  On 64-bit architectures, the pointer is 64 bits and the
   integer only 32.  The compiler warns us of loss of precision.  However we
   know we casted a normal 32-bit integer into this pointer a few
   microseconds earlier, so it is ok.  Nothing to worry about.  */
#define POINTER_TO_INT(p) ((int)(long)(p))

static void float_formatter(
    GtkTreeViewColumn * tree_column ATTRIBUTE_UNUSED,
    GtkCellRenderer * cell,
    GtkTreeModel * tree_model,
    GtkTreeIter * iter,
    gpointer data)
{
    gfloat f;
    gchar text[64];
    gtk_tree_model_get(tree_model, iter, POINTER_TO_INT(data), &f, -1);
    sprintf(text, "%.2f", f);
    g_object_set(cell, "text", text, NULL);
}

static void percent_formatter(
    GtkTreeViewColumn * tree_column ATTRIBUTE_UNUSED,
    GtkCellRenderer * cell,
    GtkTreeModel * tree_model,
    GtkTreeIter * iter,
    gpointer data)
{
    gfloat f;
    gchar text[64];
    gtk_tree_model_get(tree_model, iter, POINTER_TO_INT(data), &f, -1);
    sprintf(text, "%.1f%%", f);
    g_object_set(cell, "text", text, NULL);
}

static void TreeViewCreate(
    struct mtr_ctl *ctl)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    ReportStore = gtk_list_store_new(N_COLS,
#ifdef HAVE_IPINFO
                                     G_TYPE_STRING,
#endif
                                     G_TYPE_STRING,
                                     G_TYPE_STRING,
                                     G_TYPE_FLOAT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_FLOAT, G_TYPE_STRING);

    ReportTreeView =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(ReportStore));

    g_signal_connect(GTK_OBJECT(ReportTreeView), "button_press_event",
                     G_CALLBACK(ReportTreeView_clicked), ctl);

#ifdef HAVE_IPINFO
    if (is_printii(ctl)) {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ASN",
                                                          renderer,
                                                          "text", COL_ASN,
                                                          "foreground",
                                                          COL_COLOR, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);
    }
#endif

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Hostname",
                                                      renderer,
                                                      "text", COL_HOSTNAME,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    column = gtk_tree_view_column_new_with_attributes("Address",
                                                      renderer,
                                                      "text", COL_ADDRESS,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Loss",
                                                      renderer,
                                                      "text", COL_LOSS,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            percent_formatter,
                                            (void *) COL_LOSS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Snt",
                                                      renderer,
                                                      "text", COL_SNT,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Last",
                                                      renderer,
                                                      "text", COL_LAST,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Avg",
                                                      renderer,
                                                      "text", COL_AVG,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Best",
                                                      renderer,
                                                      "text", COL_BEST,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Worst",
                                                      renderer,
                                                      "text", COL_WORST,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("StDev",
                                                      renderer,
                                                      "text", COL_STDEV,
                                                      "foreground",
                                                      COL_COLOR, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            float_formatter,
                                            (void *) COL_STDEV, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ReportTreeView), column);

}

/*unsigned long getValue( 获取文件中指定的16进制串的值，并返回
FILE *fp, 指定文件指针
unsigned long start, 指定文件偏移量
int length) 获取的16进制字符个数/长度
*/
static unsigned long getValue(FILE *fp, unsigned long start, int length)
{
    unsigned long variable = 0;
    long val[255], i;

    fseek(fp, start, SEEK_SET);
    for (i = 0; i<length; i++)
    {
        /*过滤高位，一次读取一个字符*/
        val[i] = fgetc(fp) & 0x000000FF;
    }
    for (i = length - 1; i >= 0; i--)
    {
        /*因为读取多个16进制字符，叠加*/
        variable = variable * 0x100 + val[i];
    }
    return variable;
};

/*void getHead( 读取索引部分的范围（在文件头中，最先的2个8位16进制）
FILE *fp, 指定文件指针
unsigned long *start, 文件偏移量，索引的起止位置
unsigned long *end) 文件偏移量，索引的结束位置
*/
static void getHead(FILE *fp, unsigned long *start, unsigned long *end)
{
    /*索引的起止位置的文件偏移量，存储在文件头中的前8个16进制中
    设置偏移量为0，读取4个字符*/
    *start = getValue(fp, 0L, 4);
    /*索引的结束位置的文件偏移量，存储在文件头中的第8个到第15个的16进制中
    设置偏移量为4个字符，再读取4个字符*/
    *end = getValue(fp, 4L, 4);
};

/*int getString( 获取文件中指定的字符串，返回字符串长度
FILE *fp, 指定文件指针
unsigned long start, 指定文件偏移量
char **string) 用来存放将读取字符串的字符串空间的首地址
*/
int getString(FILE *fp, unsigned long start, char **string)
{
    unsigned long i = 0;
    char val;
    fseek(fp, start, SEEK_SET);
    /*读取字符串，直到遇到0x00为止*/
    do
    {
        val = fgetc(fp);
        /*依次放入用来存储的字符串空间中*/
        *(*string + i) = val;
        i++;
    } while (val != 0x00);
    /*返回字符串长度*/
    return i;
};

/*unsigned long searchIP( 搜索指定IP在索引区的位置，采用二分查找法；
返回IP在索引区域的文件偏移量
一条索引记录的结果是，前4个16进制表示起始IP地址
后面3个16进制，表示该起始IP在IP信息段中的位置，文件偏移量
FILE *fp,
unsigned long index_start, 索引起始位置的文件偏移量
unsigned long index_end, 索引结束位置的文件偏移量
unsigned long ip) 关键字，要索引的IP
*/
unsigned long searchIP(FILE *fp, unsigned long index_start, \

                       unsigned long index_end, uint32_t ip)
{
    unsigned long index_current, index_top, index_bottom;
    unsigned long record;
    index_bottom = index_start;
    index_top = index_end;
    /*此处的7，是因为一条索引记录的长度是7*/
    index_current = ((index_top - index_bottom) / 7 / 2) * 7 + index_bottom;
    /*二分查找法*/
    do{
        record = getValue(fp, index_current, 4);
        if (record>ip)
        {
            index_top = index_current;
            index_current = ((index_top - index_bottom) / 14) * 7 + index_bottom;
        }
        else
        {
            index_bottom = index_current;
            index_current = ((index_top - index_bottom) / 14) * 7 + index_bottom;
        }
    } while (index_bottom<index_current);
    /*返回关键字IP在索引区域的文件偏移量*/
    return index_current;
};

/*void getAddress( 读取指定IP的国家位置和地域位置
FILE *fp, 指定文件指针
unsigned long start, 指定IP在索引中的文件偏移量
char **country, 用来存放国家位置的字符串空间的首地址
char **location) 用来存放地域位置的字符串空间的首地址
*/
void getAddress(FILE *fp, unsigned long start, char **country, char **location)
{
    unsigned long redirect_address, counrty_address, location_address;
    char val;

    start += 4;
    fseek(fp, start, SEEK_SET);
    /*读取首地址的值*/
    val = (fgetc(fp) & 0x000000FF);

    if (val == REDIRECT_MODE_1)
    {
        /*重定向1类型的*/
        redirect_address = getValue(fp, start + 1, 3);
        fseek(fp, redirect_address, SEEK_SET);
        /*混合类型，重定向1类型进入后遇到重定向2类型
        读取重定向后的内容，并设置地域位置的文件偏移量*/
        if ((fgetc(fp) & 0x000000FF) == REDIRECT_MODE_2)
        {
            counrty_address = getValue(fp, redirect_address + 1, 3);
            location_address = redirect_address + 4;
            getString(fp, counrty_address, country);
        }
            /*读取重定向1后的内容，并设置地域位置的文件偏移量*/
        else
        {
            counrty_address = redirect_address;
            location_address = redirect_address + getString(fp, counrty_address, country);
        }
    }
        /*重定向2类型的*/
    else if (val == REDIRECT_MODE_2)
    {
        counrty_address = getValue(fp, start + 1, 3);
        location_address = start + 4;
        getString(fp, counrty_address, country);
    }
    else
    {
        counrty_address = start;
        location_address = counrty_address + getString(fp, counrty_address, country);
    }

    /*读取地域位置*/
    fseek(fp, location_address, SEEK_SET);
    if ((fgetc(fp) & 0x000000FF) == REDIRECT_MODE_2 || (fgetc(fp) & 0x000000FF) == REDIRECT_MODE_1)
    {
        location_address = getValue(fp, location_address + 1, 3);
    }
    getString(fp, location_address, location);

    return;
};

static void update_tree_row(
    struct mtr_ctl *ctl,
    int row,
    GtkTreeIter * iter)
{
    ip_t *addr;
    char str[256] = "???", str2[256] = "???", *name = str;

    FILE *fp;
    iconv_t conv_desc;
    unsigned long index_start, index_end, current;
    char *country;
    char *location;
    char *out_string;
    char *out_string2;
    size_t len;
    size_t utf8len;
    uint32_t haddr;
    int rc;
//    unsigned int i=0;
    char   *inptr;
    char   *outptr;

    country = (char*)malloc(256);
    location = (char*)malloc(256);
    out_string = (char*)malloc(256);
    out_string2 = (char*)malloc(256);

//    memset(country,0,255);
//    memset(location,0,255);
//    memset(out_string,0,255);

    addr = net_addr(row);
    haddr = addr->s_addr;

//    printf("%s___\n", inet_ntoa(*addr));
//    printf("%u________\n", haddr);

    haddr = ((haddr >> 24) & 0xff) | (((haddr >> 16) & 0xff) << 8) | (((haddr >> 8) & 0xff) << 16) | ((haddr & 0xff) << 24);
//    printf("%u________\n", haddr);

    fp = fopen(QQWRY, "rb");
    if(fp) {
        getHead(fp, &index_start, &index_end);
        getAddress(fp, getValue(fp, index_end + 4, 3), &country, &location);
        //搜索IP在索引区域的条目的偏移量
        current = searchIP(fp, index_start, index_end, haddr);
        //获取该IP对因的国家地址和地域地址
        getAddress(fp, getValue(fp, current + 4, 3), &country, &location);

//        printf("len: %d__\n", (int)strlen(country));
//        for(i=0;i<strlen(country);i++) {
//            printf("%X", country[i] & 0xFF);
//        }

        conv_desc = iconv_open ("utf-8", "gb2312");
        if (conv_desc == (iconv_t)-1) {
            printf("iconv_open error!");
        } else {
            inptr = location;
            outptr = out_string;
            len = strlen(location) * 4;
            utf8len = len;
            rc = iconv(conv_desc, &inptr, &len, &outptr, &utf8len);
            if (rc == -1) {
//                switch (errno) {
//                    /* See "man 3 iconv" for an explanation. */
//                    case EILSEQ:
//                        fprintf (stderr, "Invalid multibyte sequence.\n");
//                        break;
//                    case EINVAL:
//                        fprintf (stderr, "Incomplete multibyte sequence.\n");
//                        break;
//                    case E2BIG:
//                        fprintf (stderr, "No more room.\n");
//                        break;
//                    default:
//                        fprintf (stderr, "Error: %s.\n", strerror (errno));
//                }
//                // exit ok
////                exit (1);
            }

            inptr = country;
            outptr = out_string2;
            len = strlen(country) * 4;
            utf8len = len;
            rc = iconv(conv_desc, &inptr, &len, &outptr, &utf8len);

            snprintf(str2, sizeof(str2), "%s %s", out_string2,
                     out_string);

            iconv_close (conv_desc);
        }

//        printf("%s___\n", country);
//        printf("%s___\n", location);
//        printf("len out_string: %d \n", (int)strlen(out_string));
//        printf("%s___\n", str2);
//        printf("%s__%s\n", country, location);
        fclose(fp);
    }

    if (addrcmp((void *) addr, (void *) &ctl->unspec_addr, ctl->af)) {
        if ((name = dns_lookup(ctl, addr))) {
            if (ctl->show_ips) {
                snprintf(str, sizeof(str), "%s (%s)", name,
                         strlongip(ctl, addr));
                name = str;
            }
        } else
            name = strlongip(ctl, addr);
    }

    gtk_list_store_set(ReportStore, iter,
                       COL_HOSTNAME, name,
                       COL_ADDRESS, str2,
                       COL_LOSS, (float) (net_loss(row) / 1000.0),
                       COL_RCV, net_returned(row),
                       COL_SNT, net_xmit(row),
                       COL_LAST, net_last(row) / 1000,
                       COL_BEST, net_best(row) / 1000,
                       COL_AVG, net_avg(row) / 1000,
                       COL_WORST, net_worst(row) / 1000,
                       COL_STDEV, (float) (net_stdev(row) / 1000.0),
                       COL_COLOR, net_up(row) ? NULL : "red", -1);
#ifdef HAVE_IPINFO
    if (is_printii(ctl))
        gtk_list_store_set(ReportStore, iter, COL_ASN,
                           fmt_ipinfo(ctl, addr), -1);
#endif
}

void gtk_redraw(
    struct mtr_ctl *ctl)
{
    int max = net_max(ctl);

    GtkTreeIter iter;
    int row = net_min(ctl);
    gboolean valid;

    valid =
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ReportStore), &iter);

    while (valid) {
        if (row < max) {
            update_tree_row(ctl, row++, &iter);
            valid =
                gtk_tree_model_iter_next(GTK_TREE_MODEL(ReportStore),
                                         &iter);
        } else {
            valid = gtk_list_store_remove(ReportStore, &iter);
        }
    }
    while (row < max) {
        gtk_list_store_append(ReportStore, &iter);
        update_tree_row(ctl, row++, &iter);
    }
}


static void Window_fill(
    struct mtr_ctl *ctl,
    GtkWidget * Window)
{
    GtkWidget *VBox;
    GtkWidget *Toolbar;
    GtkWidget *scroll;

    gtk_window_set_title(GTK_WINDOW(Window), "My traceroute");
    gtk_window_set_default_size(GTK_WINDOW(Window), 650, 400);
    gtk_container_set_border_width(GTK_CONTAINER(Window), 10);
    VBox = gtk_vbox_new(FALSE, 10);

    Toolbar = gtk_hbox_new(FALSE, 10);
    Toolbar_fill(ctl, Toolbar);
    gtk_box_pack_start(GTK_BOX(VBox), Toolbar, FALSE, FALSE, 0);

    TreeViewCreate(ctl);
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
                                        GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(scroll), ReportTreeView);
    gtk_box_pack_start(GTK_BOX(VBox), scroll, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(Window), VBox);
}


void gtk_open(
    struct mtr_ctl *ctl)
{
    GdkPixbuf *icon;
    int argc = 1;
    char *args[2];
    char **argv;

    argv = args;
    argv[0] = xstrdup("");
    argv[1] = NULL;
    gtk_do_init(&argc, &argv);
    free(argv[0]);

    icon = gdk_pixbuf_new_from_xpm_data((const char **) mtr_icon);
    gtk_window_set_default_icon(icon);

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    g_set_application_name("My traceroute");

    Window_fill(ctl, main_window);

    g_signal_connect(GTK_OBJECT(main_window), "delete_event",
                     GTK_SIGNAL_FUNC(Window_destroy), NULL);
    g_signal_connect(GTK_OBJECT(main_window), "destroy",
                     GTK_SIGNAL_FUNC(Window_destroy), NULL);

    gtk_widget_show_all(main_window);
}


void gtk_close(
    void)
{
}


int gtk_keyaction(
    void)
{
    return 0;
}


static gint gtk_ping(
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    gtk_redraw(ctl);
    net_send_batch(ctl);
    net_harvest_fds(ctl);
    g_source_remove(ping_timeout_timer);
    gtk_add_ping_timeout(ctl);
    return TRUE;
}


static gboolean gtk_net_data(
    ATTRIBUTE_UNUSED GIOChannel * channel,
    ATTRIBUTE_UNUSED GIOCondition cond,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    net_process_return(ctl);
    return TRUE;
}


static gboolean gtk_dns_data(
    ATTRIBUTE_UNUSED GIOChannel * channel,
    ATTRIBUTE_UNUSED GIOCondition cond,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    dns_ack(ctl);
    gtk_redraw(ctl);
    return TRUE;
}

#ifdef ENABLE_IPV6
static gboolean gtk_dns_data6(
    ATTRIBUTE_UNUSED GIOChannel * channel,
    ATTRIBUTE_UNUSED GIOCondition cond,
    gpointer data)
{
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    dns_ack6();
    gtk_redraw(ctl);
    return TRUE;
}
#endif


void gtk_loop(
    struct mtr_ctl *ctl)
{
    GIOChannel *net_iochannel, *dns_iochannel;

    gtk_add_ping_timeout(ctl);

    net_iochannel = g_io_channel_unix_new(net_waitfd());
    g_io_add_watch(net_iochannel, G_IO_IN, gtk_net_data, ctl);
#ifdef ENABLE_IPV6
    if (dns_waitfd6() > 0) {
        dns_iochannel = g_io_channel_unix_new(dns_waitfd6());
        g_io_add_watch(dns_iochannel, G_IO_IN, gtk_dns_data6, ctl);
    }
#endif
    dns_iochannel = g_io_channel_unix_new(dns_waitfd());
    g_io_add_watch(dns_iochannel, G_IO_IN, gtk_dns_data, ctl);

    gtk_main();
}

static gboolean NewDestination_activate(
    GtkWidget * widget ATTRIBUTE_UNUSED,
    gpointer data)
{
    gchar *hostname;
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;
    GtkTreePath *path = (GtkTreePath *) ctl->gtk_data;

    hostname = getSelectedHost(path);
    if (hostname) {
        ctl->gtk_data = hostname;
        gtk_entry_set_text(GTK_ENTRY(Entry), hostname);
        Host_activate(Entry, ctl);
        g_free(hostname);
    }
    return TRUE;
}


static gboolean Copy_activate(
    GtkWidget * widget ATTRIBUTE_UNUSED,
    gpointer data)
{
    gchar *hostname;
    GtkTreePath *path = (GtkTreePath *) data;

    hostname = getSelectedHost(path);
    if (hostname != NULL) {
        GtkClipboard *clipboard;

        clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, hostname, -1);

        clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
        gtk_clipboard_set_text(clipboard, hostname, -1);

        g_free(hostname);
    }

    return TRUE;
}

static gchar *getSelectedHost(
    GtkTreePath * path)
{
    GtkTreeIter iter;
    gchar *name = NULL;

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(ReportStore), &iter, path)) {
        gtk_tree_model_get(GTK_TREE_MODEL(ReportStore), &iter,
                           COL_HOSTNAME, &name, -1);
    }
    gtk_tree_path_free(path);
    return name;
}


static gboolean ReportTreeView_clicked(
    GtkWidget * Tree ATTRIBUTE_UNUSED,
    GdkEventButton * event,
    gpointer data)
{
    GtkWidget *popup_menu;
    GtkWidget *copy_item;
    GtkWidget *newdestination_item;
    GtkTreePath *path;
    struct mtr_ctl *ctl = (struct mtr_ctl *) data;

    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(ReportTreeView),
                                       event->x, event->y, &path, NULL,
                                       NULL, NULL))
        return FALSE;

    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ReportTreeView), path, NULL,
                             FALSE);

    /* Single right click: prepare and show the popup menu */
    popup_menu = gtk_menu_new();

    copy_item = gtk_menu_item_new_with_label("Copy to clipboard");
    newdestination_item =
        gtk_menu_item_new_with_label("Set as new destination");

    gtk_menu_append(GTK_MENU(popup_menu), copy_item);
    gtk_menu_append(GTK_MENU(popup_menu), newdestination_item);

    g_signal_connect(GTK_OBJECT(copy_item), "activate",
                     GTK_SIGNAL_FUNC(Copy_activate), path);

    ctl->gtk_data = path;
    g_signal_connect(GTK_OBJECT(newdestination_item), "activate",
                     GTK_SIGNAL_FUNC(NewDestination_activate), ctl);

    gtk_widget_show(copy_item);
    gtk_widget_show(newdestination_item);

    gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL,
                   0, event->time);
    return TRUE;
}
