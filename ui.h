#ifndef UI_H
#define UI_H

typedef int uid;
#define UID_NULL 0

struct rect {
    int x, y, w, h;
};

struct ui {
    uint32_t color;
    struct renderer *renderer;
    struct rect hoverable;
    uid rect_id;
};

enum color {
    CL_BLACK = 0xFF000000,
    CL_WHITE = 0xFFFFFFFF
};

struct char_pixels {
    uint64_t pixels;
    int width;
    int height;
};

struct char_pixels g_char_maps[128];

void make_char_pixels(const char *data, struct char_pixels *out) {
    //assert(strlen(data) == 64);
    out->pixels = 0;
    for (int i = 0; i < 25; i++) {
        uint64_t v = *(data + i) == '#' ? 0x1ULL : 0x0ULL;
        out->pixels |= v << i;
    }
    out->width = 5;
    out->height = 5;
}

struct cell { 
    int row, col;
};

int ui_draw_char(struct ui *ui, char c, struct cell cell) {
    int max_col = 0;

    for (int i = 0; i < 64; i++) {
        int row = i / g_char_maps[c].width;
        int col = i % g_char_maps[c].width;
        uint64_t v = (g_char_maps[c].pixels >> i) & 0x1;

        if (v == 1) {
            max_col = col > max_col ? col : max_col;
            int px_idx = (cell.row - row) * ui->renderer->width + (cell.col + col);
            //ui->renderer->pixels[px_idx] = 0xFF000000 | (red << 16) | (green << 8) | (blue << 0);
            ui->renderer->pixels[px_idx] = ui->color;
        }
    }
    
    return max_col + 1;
}

int ui_draw_text(struct ui *ui, const char *text, struct cell cell) {
    cell.row = ui->renderer->height - cell.row - 1;

    int off = 0;
    const char *c = text;
    while (*c != '\0') {
        if (*c == ' ') {
            off += 4;
        } else {
            off += ui_draw_char(ui, *c, (struct cell) { cell.row, cell.col + off });
            off += 1; //add space between characters
        }
        c++;
    }
    return off;
}

void init_char_map() {

    make_char_pixels(" ### "  
                     "#   #"
                     "#####"
                     "#   #"
                     "#   #",
                     &g_char_maps['A']);

    make_char_pixels("#### "
                     "#   #"
                     "#### "
                     "#   #"
                     "#### ",
                     &g_char_maps['B']);
                     
    make_char_pixels(" ### "
                     "#    "
                     "#    "
                     "#    "
                     " ### ",
                     &g_char_maps['C']);
                     
    make_char_pixels("#### "
                     "#   #"
                     "#   #"
                     "#   #"
                     "#### ",
                     &g_char_maps['D']);
                     
    make_char_pixels("#####"
                     "#    "
                     "#### "
                     "#    "
                     "#####",
                     &g_char_maps['E']);
                      
    make_char_pixels("#####"
                     "#    "
                     "#### "
                     "#    "
                     "#    ",
                     &g_char_maps['F']);
                      
    make_char_pixels(" ### "
                     "#    "
                     "#  ##"
                     "#   #"
                     " ### ",
                     &g_char_maps['G']);
                      
    make_char_pixels("#   #"
                     "#   #"
                     "#####"
                     "#   #"
                     "#   #",
                     &g_char_maps['H']);
                      
    make_char_pixels("###  "
                     " #   "
                     " #   "
                     " #   "
                     "###  ",
                     &g_char_maps['I']);
                      
    make_char_pixels("  ###"
                     "   # "
                     "   # "
                     "#  # "
                     " ##  ",
                     &g_char_maps['J']);
                      
    make_char_pixels("#  # "
                     "# #  "
                     "##   "
                     "# #  "
                     "#  # ",
                     &g_char_maps['K']);
                      
    make_char_pixels("#    "
                     "#    "
                     "#    "
                     "#    "
                     "#####",
                     &g_char_maps['L']);
                      
    make_char_pixels("#   #"
                     "## ##"
                     "# # #"
                     "#   #"
                     "#   #",
                     &g_char_maps['M']);
                      
    make_char_pixels("#   #"
                     "##  #"
                     "# # #"
                     "#  ##"
                     "#   #",
                     &g_char_maps['N']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     "#   #"
                     "#   #"
                     " ### ",
                     &g_char_maps['O']);
                      
    make_char_pixels("#### "
                     "#   #"
                     "#### "
                     "#    "
                     "#    ",
                     &g_char_maps['P']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     "#   #"
                     "#  ##"
                     " ####",
                     &g_char_maps['Q']);
                      
    make_char_pixels("#### "
                     "#   #"
                     "#### "
                     "# #  "
                     "#  # ",
                     &g_char_maps['R']);
                      
    make_char_pixels(" ####"
                     "#    "
                     " ### "
                     "    #"
                     "#### ",
                     &g_char_maps['S']);
                      
    make_char_pixels("#####"
                     "  #  "
                     "  #  "
                     "  #  "
                     "  #  ",
                     &g_char_maps['T']);
                      
    make_char_pixels("#   #"
                     "#   #"
                     "#   #"
                     "#   #"
                     " ### ",
                     &g_char_maps['U']);
                      
    make_char_pixels("#   #"
                     "#   #"
                     "#   #"
                     " # # "
                     "  #  ",
                     &g_char_maps['V']);
                      
    make_char_pixels("#   #"
                     "#   #"
                     "# # #"
                     "## ##"
                     "#   #",
                     &g_char_maps['W']);
                      
    make_char_pixels("#   #"
                     " # # "
                     "  #  "
                     " # # "
                     "#   #",
                     &g_char_maps['X']);
                      
    make_char_pixels("#   #"
                     " # # "
                     "  #  "
                     "  #  "
                     "  #  ",
                     &g_char_maps['Y']);
                      
    make_char_pixels("#####"
                     "   # "
                     "  #  "
                     " #   "
                     "#####",
                     &g_char_maps['Z']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     "#   #"
                     "#   #"
                     " ### ",
                     &g_char_maps['0']);
                      
    make_char_pixels(" #   "
                     "##   "
                     " #   "
                     " #   "
                     "###  ",
                     &g_char_maps['1']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     "   # "
                     "  #  "
                     "#####" ,
                     &g_char_maps['2']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     "   # "
                     "#   #"
                     " ### ",
                     &g_char_maps['3']);
                      
    make_char_pixels("#   #"
                     "#   #"
                     "#####"
                     "    #"
                     "    #",
                     &g_char_maps['4']);
                      
    make_char_pixels("#####"
                     "#    "
                     "#### "
                     "    #"
                     "#### ",
                     &g_char_maps['5']);
                      
    make_char_pixels(" ### "
                     "#    "
                     "#### "
                     "#   #"
                     " ### ",
                     &g_char_maps['6']);
                      
    make_char_pixels("#####"
                     "   # "
                     "  #  "
                     " #   "
                     "#    ",
                     &g_char_maps['7']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     " ### "
                     "#   #"
                     " ### ",
                     &g_char_maps['8']);
                      
    make_char_pixels(" ### "
                     "#   #"
                     " ####"
                     "    #"
                     " ### ",
                     &g_char_maps['9']);
 
    make_char_pixels("  #  "
                     " #   "
                     " #   "
                     "#    "
                     "#    ",
                     &g_char_maps['/']);

    make_char_pixels("     "
                     "#    "
                     "     "
                     "     "
                     "#    ",
                     &g_char_maps[':']);
}

void ui_draw_horizontal_line(struct ui *ui, int startx, int endx, int row) {
    for (int col = startx; col < endx; col++) {
        int px_idx = row * ui->renderer->width + col;
        ui->renderer->pixels[px_idx] = ui->color;
    }
}

void ui_draw_vertical_line(struct ui *ui, int starty, int endy, int col) {
    for (int row = starty; row < endy; row++) {
        int px_idx = row * ui->renderer->width + col;
        ui->renderer->pixels[px_idx] = ui->color;
    }
}

void ui_draw_rect(struct ui* ui, struct cell pos, struct cell dim) {
    pos.row = ui->renderer->height - pos.row - 1;

    {
        int startx = pos.col;
        int endx = pos.col + dim.col;
        ui_draw_horizontal_line(ui, startx, endx, pos.row);
    }
    {
        int startx = pos.col;
        int endx = pos.col + dim.col;
        ui_draw_horizontal_line(ui, startx, endx, pos.row - dim.row);
    }

    {
        int starty = pos.row - dim.row;
        int endy = pos.row;
        ui_draw_vertical_line(ui, starty, endy, pos.col);
    }
    {
        int starty = pos.row - dim.row;
        int endy = pos.row;
        ui_draw_vertical_line(ui, starty, endy, pos.col + dim.col);
    }
}

uid ui_hovered_rects(struct ui *ui, int x, int y) {
    y = ui->renderer->height - y - 1;
    if (x >= ui->hoverable.x && x < ui->hoverable.x + ui->hoverable.w &&
        y >= ui->hoverable.y && y < ui->hoverable.y + ui->hoverable.h) {
        
        return 1;
    }

    return UID_NULL;
}

uid ui_draw_hoverable_rect(struct ui *ui, struct cell pos, struct cell dim) {
    ui_draw_rect(ui, pos, dim);
    ui->hoverable.x = pos.col;
    ui->hoverable.y = ui->renderer->height - pos.row - 1 - dim.row;
    ui->hoverable.w = dim.col;
    ui->hoverable.h = dim.row;
    return 1;
}

void ui_set_color(struct ui *ui, enum color color) {
    ui->color = color;
}

void ui_begin(struct ui *ui) {
    ui->rect_id = UID_NULL;
}

void ui_init(struct ui *ui, struct renderer *r) {
    ui->color = CL_BLACK;
    ui->renderer = r;
    init_char_map();
    ui->rect_id = UID_NULL;
}


#endif 
