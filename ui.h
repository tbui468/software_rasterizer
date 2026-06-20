#ifndef UI_H
#define UI_H

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

int ui_draw_char(struct renderer *r, char c, struct cell cell) {
    uint8_t red = 0; 
    uint8_t green = 0; 
    uint8_t blue = 0; 

    int max_col = 0;

    for (int i = 0; i < 64; i++) {
        int row = i / g_char_maps[c].width;
        int col = i % g_char_maps[c].width;
        uint64_t v = (g_char_maps[c].pixels >> i) & 0x1;

        if (v == 1) {
            max_col = col > max_col ? col : max_col;
            int px_idx = (cell.row - row) * r->width + (cell.col + col);
            r->pixels[px_idx] = 0xFF000000 | (red << 16) | (green << 8) | (blue << 0);
        }
    }
    
    return max_col + 1;
}

int ui_draw_text(struct renderer *r, const char *text, struct cell cell) {
    cell.row = r->height - cell.row - 1;

    int off = 0;
    const char *c = text;
    while (*c != '\0') {
        if (*c == ' ') {
            off += 4;
        } else {
            off += ui_draw_char(r, *c, (struct cell) { cell.row, cell.col + off });
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

}

#endif 
