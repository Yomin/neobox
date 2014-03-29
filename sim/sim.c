/*
 * Copyright (c) 2013, 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <unistd.h>
#include <linux/input.h>

#ifdef NDEBUG
#   define DEBUG
#else
#   define DEBUG(x) x
#endif

#define SCREEN_DEV    "screen.ipc"
#define AUX_DEV       "aux.ipc"
#define POWER_DEV     "power.ipc"
#define PIXEL_OFFSET  100
#define PIXEL_MAX     930
#define SCREEN_YALIGN(pos) ((pos)*((PIXEL_MAX-PIXEL_OFFSET)/(FB_VINFO_YRES*1.0)) + PIXEL_OFFSET)
#define SCREEN_XALIGN(pos) ((pos)*((PIXEL_MAX-PIXEL_OFFSET)/(FB_VINFO_XRES*1.0)) + PIXEL_OFFSET)

#define FB_DEV "fb.ipc"
#define FB_SHM "/fbsim"
#define FB_VINFO_BITS_PER_PIXEL 16
#define FB_VINFO_XRES 240 // 480
#define FB_VINFO_YRES 320 // 640
#define FB_VINFO_XOFFSET 0
#define FB_VINFO_YOFFSET 0
#define FB_FINFO_LINE_LENGTH 480 // 960
#define FB_SIZE (FB_VINFO_XRES*FB_VINFO_YRES*(FB_VINFO_BITS_PER_PIXEL/8))

int screen, aux, power, fb_sock, fb_shm;
unsigned char *fb_ptr;

GtkWidget *window;
cairo_surface_t *surface;
int stride, width, height, landscape;
int pressed_screen, pressed_aux, pressed_power;

void cleanup()
{
    printf("Cleanup\n");
    close(screen);
    close(aux);
    close(power);
    close(fb_sock);
    shm_unlink(FB_SHM);
    cairo_surface_destroy(surface);
}

void signal_handler(int signal)
{
    switch(signal)
    {
    case SIGINT:
        cleanup();
        exit(0);
    case SIGPIPE:
        printf("sigpipe\n");
    }
}

void send_event(int fd, int type, int code, int value)
{
    struct input_event event;
    
    event.type = type;
    event.code = code;
    event.value = value;
    
    write(fd, &event, sizeof(struct input_event));
}

void send_fb_info(int fd)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    vinfo.bits_per_pixel = FB_VINFO_BITS_PER_PIXEL;
    vinfo.xres = FB_VINFO_XRES;
    vinfo.yres = FB_VINFO_YRES;
    vinfo.xoffset = FB_VINFO_XOFFSET;
    vinfo.yoffset = FB_VINFO_YOFFSET;
    finfo.line_length = FB_FINFO_LINE_LENGTH;
    
    const char *shm = FB_SHM;
    
    send(fd, shm, 20, 0);
    send(fd, &vinfo, sizeof(struct fb_var_screeninfo), 0);
    send(fd, &finfo, sizeof(struct fb_fix_screeninfo), 0);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    return TRUE;
}

gboolean on_button_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GdkEventButton *button_event = (GdkEventButton*)event;
    int x = landscape ? button_event->y : button_event->x;
    int y = landscape ? button_event->x : button_event->y;
    
    printf("Pressed\n");
    
    if(y >= 0 && y < height && x >= 0 && x < width)
    {
        if(landscape)
        {
            x = SCREEN_YALIGN(button_event->y);
            y = SCREEN_XALIGN(button_event->x);
        }
        else
        {
            x = SCREEN_XALIGN(button_event->x);
            y = SCREEN_YALIGN(height - button_event->y);
        }
        send_event(screen, EV_ABS, ABS_Y, y);
        send_event(screen, EV_ABS, ABS_X, x);
        send_event(screen, EV_KEY, BTN_TOUCH, 1);
    }
    
    pressed_screen = 1;
    
    return FALSE;
}

gboolean on_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    printf("Released\n");
    
    send_event(screen, EV_KEY, BTN_TOUCH, 0);
    pressed_screen = 0;
    
    return FALSE;
}

gboolean on_key_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    switch(event->key.keyval)
    {
    case GDK_KEY_a:
        if(!pressed_aux)
        {
            printf("AUX pressed\n");
            pressed_aux = 1;
            send_event(aux, EV_KEY, KEY_PHONE, 1);
        }
        break;
    case GDK_KEY_p:
        if(!pressed_power)
        {
            printf("Power pressed\n");
            pressed_power = 1;
            send_event(power, EV_KEY, KEY_POWER, 1);
        }
        break;
    }
    
    return FALSE;
}

gboolean on_key_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    switch(event->key.keyval)
    {
    case GDK_KEY_a:
        printf("AUX released\n");
        pressed_aux = 0;
        send_event(aux, EV_KEY, KEY_PHONE, 0);
        break;
    case GDK_KEY_p:
        printf("Power released\n");
        pressed_power = 0;
        send_event(power, EV_KEY, KEY_POWER, 0);
        break;
    }
    
    return FALSE;
}

gboolean on_move(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GdkEventButton *button_event = (GdkEventButton*)event;
    int x = landscape ? button_event->y : button_event->x;
    int y = landscape ? button_event->x : button_event->y;
    
    if(pressed_screen && y >= 0 && y < height && x >= 0 && x < width)
    {
        if(landscape)
        {
            x = SCREEN_YALIGN(button_event->y);
            y = SCREEN_XALIGN(button_event->x);
        }
        else
        {
            x = SCREEN_XALIGN(button_event->x);
            y = SCREEN_YALIGN(height - button_event->y);
        }
        send_event(screen, EV_ABS, ABS_Y, y);
        send_event(screen, EV_ABS, ABS_X, x);
    }
    
    return FALSE;
}

gboolean tkbio_event(GIOChannel *channel, GIOCondition cond, gpointer data)
{
    char tmp;
    if(cond == G_IO_IN)
    {
        g_io_channel_read_chars(channel, &tmp, 1, NULL, NULL);
        cairo_surface_mark_dirty(surface);
        gtk_widget_queue_draw(window);
        return TRUE;
    }
    printf("Disconnect [%i]\n", g_io_channel_unix_get_fd(channel));
    return FALSE;
}

gboolean fb_event(GIOChannel *channel, GIOCondition cond, gpointer data)
{
    int tkbio;
    if((tkbio = accept(fb_sock, 0, 0)) != -1)
    {
        printf("Connect [%i]\n", tkbio);
        GIOChannel *tkbio_chan = g_io_channel_unix_new(tkbio);
        g_io_add_watch(tkbio_chan, G_IO_IN|G_IO_HUP, tkbio_event, NULL);
        send_fb_info(tkbio);
    }
    return TRUE;
}

int main(int argc, char* argv[])
{
    int opt;
    
    landscape = 0;
    width = FB_VINFO_XRES;
    height = FB_VINFO_YRES;

    while((opt = getopt(argc, argv, "l")) != -1)
    {
        switch(opt)
        {
        case 'l':
            landscape = 1;
            width = FB_VINFO_YRES;
            height = FB_VINFO_XRES;
            break;
        default:
            printf("Usage: %s [-l]\n", argv[0]);
            return 0;
        }
    }
    
    if(mkfifo(SCREEN_DEV, 0664) == -1 && errno != EEXIST)
    {
        perror("Failed to create screen socket");
        return 1;
    }
    
    if(mkfifo(AUX_DEV, 0664) == -1 && errno != EEXIST)
    {
        perror("Failed to create aux socket");
        return 2;
    }
    
    if(mkfifo(POWER_DEV, 0664) == -1 && errno != EEXIST)
    {
        perror("Failed to create power socket");
        return 3;
    }
    
    printf("Waiting for tsp\n");
    
    if((screen = open(SCREEN_DEV, O_WRONLY)) == -1)
    {
        perror("Failed to open screen socket");
        return 4;
    }
    
    if((aux = open(AUX_DEV, O_WRONLY)) == -1)
    {
        perror("Failed to open aux socket");
        return 5;
    }
    
    if((power = open(POWER_DEV, O_WRONLY)) == -1)
    {
        perror("Failed to open power socket");
        return 6;
    }
    
    printf("Connected\n");
    
    if((fb_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to open fb socket");
        return 7;
    }
    
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, FB_DEV);
    
    if(unlink(addr.sun_path) == -1 && errno != ENOENT)
    {
        perror("Failed to unlink fb socket file");
        return 8;
    }
    
    if(bind(fb_sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Failed to bind fb socket");
        return 9;
    }
    
    listen(fb_sock, 0);
    
    if((fb_shm = shm_open(FB_SHM, O_CREAT|O_RDWR, 0644)) == -1)
    {
        perror("Failed to open shared memory");
        return 10;
    }
    
    if(ftruncate(fb_shm, FB_SIZE) == -1)
    {
        perror("Failed to truncate shared memory");
        return 11;
    }
    
    fb_ptr = mmap(0, FB_SIZE, PROT_READ, MAP_SHARED, fb_shm, 0);
    if(fb_ptr == MAP_FAILED)
    {
        perror("Failed to mmap shared memory");
        return 12;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);
    
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, width);
    surface = cairo_image_surface_create_for_data(
        fb_ptr, CAIRO_FORMAT_RGB16_565, width, height, stride);
    
    pressed_screen = 0;
    pressed_aux = 0;
    pressed_power = 0;
    
    gtk_init(&argc, &argv);
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Neobox Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    
    gtk_widget_set_events(window, GDK_POINTER_MOTION_MASK|
        GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_move), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(window, "key-release-event", G_CALLBACK(on_key_release), NULL);
    
    GIOChannel *fb_chan = g_io_channel_unix_new(fb_sock);
    g_io_add_watch(fb_chan, G_IO_IN, fb_event, NULL);
    
    gtk_widget_show(window);
    
    gtk_main();
    
    cleanup();
    
    return 0;
}
