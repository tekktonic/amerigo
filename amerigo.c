#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <jansson.h>
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

enum TextureRenderEnum
{
    TR_MAIN,
    TR_EDIT
};

// tile is a tile in the tileset
struct Tile
{
    int x;
    int y;
};

// maptile is a tile on the map being edited
struct MapTile
{
    struct Tile tile;
    int x;
    int y;
    bool exists;
};



int numVTiles, numHTiles;
int tileSize;
SDL_Texture *tileSets[64][2];
SDL_Rect selected;

TTF_Font *fnt;

int currentTileSet = 0;

struct Tile currentTile = {0, 0};

json_t *map;
json_t *jsonTiles;

char *mapname;

SDL_Window *mainWindow;
SDL_Renderer *renderer;

SDL_Window *editWindow;
SDL_Renderer *editRenderer;

void finishup()
{
    SDL_Quit();
    IMG_Quit();
    exit(0);
}

int textinput_int(SDL_Renderer *r)
{
    size_t bufsiz = 1024;
    size_t bufused = 0;
    char *buffer = malloc(bufsiz);
    int ret = 0;

    buffer[0] = '\0';
    
    SDL_StartTextInput();

    SDL_Rect rt = {0, 0, 500, 100};
    
    while(true)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                // breaks out of loop.
                finishup();
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_RETURN)
                    goto done;
                else if (event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    if (bufused > 0)
                    {
                        buffer[bufused - 1] = '\0';
                        bufused--;
                    }
                    else
                        buffer[bufused] = '\0';
                        
                }
                break;
                    
            case SDL_TEXTINPUT:
                if (bufused + strlen(event.text.text) >= bufsiz)
                {
                    bufsiz *= 2;
                    buffer = realloc(buffer, bufsiz);
                }
                for (char *txt = event.text.text; *txt; txt++)
                {
                    if (isdigit(*txt))
                        buffer[bufused++] = *txt;
                }
//                strncat(buffer, event.text.text, bufsiz - 1);
//                bufused += strlen(event.text.text);
                buffer[bufsiz - 1] = '\0';
                break;
            }
        }
    }
done:
    SDL_StopTextInput();
    sscanf(buffer, "%d", &ret);

    return ret;
}

char *textinput_string(SDL_Renderer *target)
{
    size_t bufsiz = 1024;
    size_t bufused = 0;
    char *buffer = malloc(bufsiz);
    buffer[0] = '\0';
    
    SDL_StartTextInput();
    SDL_Rect ir = {.x = 0, .y = 0, .w = 500, .h = 100};
    
    while(true)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_WINDOWEVENT:
                // breaks out of loop.
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    finishup();
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_RETURN)
                    goto done;
                else if (event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    if (bufused > 0)
                    {
                        buffer[bufused - 1] = '\0';
                        bufused--;
                    }
                    else
                        buffer[bufused] = '\0';
                        
                }
                break;
                    
            case SDL_TEXTINPUT:
                if (bufused + strlen(event.text.text) > bufsiz)
                {
                    bufsiz *= 2;
                    buffer = realloc(buffer, bufsiz);
                }
                strncat(buffer, event.text.text, bufsiz - 1);
                bufused += strlen(event.text.text);

                buffer[bufsiz - 1] = '\0';
                break;
            }
            SDL_Surface *tmpsurf = TTF_RenderUTF8_Blended(fnt, buffer, (SDL_Color){.r = 255, .g = 255, .b = 255, .a = 255});
            SDL_Texture *tmptex = SDL_CreateTextureFromSurface(target, tmpsurf);
            SDL_FreeSurface(tmpsurf);
            SDL_RenderClear(target);
            SDL_QueryTexture(tmptex, NULL, NULL, &ir.w, &ir.h);
            SDL_RenderCopy(target, tmptex, NULL, &ir);
            SDL_RenderPresent(target);
        }
    }
done:
    SDL_StopTextInput();
    return buffer;
}

int save()
{
    if (!mapname)
    {
        mapname = textinput_string(renderer);
    }
    printf("%d\n", json_array_size(jsonTiles));
    json_object_set(map, "map", jsonTiles);

    printf("%s\n", mapname);
    json_dump_file(map, mapname, JSON_INDENT(4));
    return 1;
//    return json_dump_file(map, mapname, JSON_INDENT(4));
}

void fill(struct MapTile *tiles)
{
    for (int i = 0; i < numHTiles * numVTiles; i++)
    {
        if (!tiles[i].exists)
        {
            tiles[i] = (struct MapTile){.exists = true, .y = i / numHTiles, .x = i % numHTiles, .tile = {.x = selected.x, .y = selected.y}};

            SDL_Rect dr = {.x = tiles[i].x * tileSize, .y = tiles[i].y * tileSize, .w = tileSize, .h = tileSize};
            SDL_Rect sr = {.x = selected.x, .y = selected.y, .w = tileSize, .h = tileSize};
            
//            SDL_RenderCopy(editRenderer, tileSets[currentTileSet][TR_EDIT], &sr, &dr);

            json_t *jtmp = json_object();
            json_object_set_new(jtmp, "tileX", json_integer(selected.x / tileSize));
            json_object_set_new(jtmp, "tileY", json_integer(selected.y / tileSize));
            json_array_set_new(jsonTiles, tiles[i].y * numHTiles + tiles[i].x, jtmp);
        }
    }
}

void clear(struct MapTile *tiles)
{
    for (int i = 0; i < numHTiles * numVTiles; i++)
    {
        tiles[i].exists = false;
        json_array_set_new(jsonTiles, i, json_null());
    }


}
int main(void)
{
    char *tileSetName;
    map = json_object();
    jsonTiles = json_array();
    

    printf("%d\n", json_array_size(jsonTiles));
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
    TTF_Init();
    mainWindow = SDL_CreateWindow("Tileset?", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 128, 0);
    renderer = SDL_CreateRenderer(mainWindow, -1, 0);
    if (!renderer)
        printf("renderer: %s\n", SDL_GetError());
    
    editWindow = SDL_CreateWindow("Edit", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 769, 768, 0);
    editRenderer = SDL_CreateRenderer(editWindow, -1, 0);

    fnt = TTF_OpenFont("font.ttf", 10);
    SDL_SetRenderDrawColor(editRenderer, 255, 255, 0, 255);


    while (!tileSets[currentTileSet][TR_MAIN] || !tileSets[currentTileSet][TR_EDIT])
    {
        
        tileSetName = textinput_string(renderer);
        
        tileSets[currentTileSet][TR_MAIN] = IMG_LoadTexture(renderer, tileSetName);
        if (!tileSets[currentTileSet][TR_MAIN])
        {
            fprintf(stderr, "Something went wrong: %s\n", SDL_GetError());
            exit(1);
        }
        tileSets[currentTileSet][TR_EDIT] = IMG_LoadTexture(editRenderer, tileSetName);
        json_object_set_new(map, "tileset", json_string(tileSetName));
    }
    
    SDL_SetWindowTitle(mainWindow, tileSetName);
    int tileSetW, tileSetH;
    SDL_QueryTexture(tileSets[currentTileSet][TR_MAIN], NULL, NULL, &tileSetW, &tileSetH);
    
    SDL_SetWindowSize(mainWindow, tileSetW, tileSetH);

    tileSize  = textinput_int(renderer);
    json_object_set_new(map, "tilewidth", json_integer(tileSize));
    json_object_set_new(map, "tileheight", json_integer(tileSize));
//    if (SDL_RenderCopy(renderer, tileSet, &tr, NULL))
//        printf("%s\n", SDL_GetError());
    SDL_RenderPresent(renderer);

    selected = (SDL_Rect){.x = 0, .y = 0, .w = tileSize, .h = tileSize};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);

    numHTiles = ceil((768 / tileSize));
    numVTiles = ceil((768 / tileSize));

    struct MapTile tiles[numHTiles * numVTiles];

    
    for (int i = 0; i < numHTiles * numVTiles; i++)
    {
        tiles[i] = (struct MapTile){.exists = false};
        json_array_append_new(jsonTiles, json_null());
    }
    printf("tiles: %d, %d\n", json_array_size(jsonTiles), numHTiles * numVTiles);

    json_object_set_new(map, "width", json_integer(numHTiles));
    json_object_set_new(map, "height", json_integer(numVTiles));
    
    SDL_Event event;
    while (true)
    {
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tileSets[currentTileSet][TR_MAIN], NULL, NULL);
        SDL_RenderFillRect(renderer, &selected);

        SDL_RenderClear(editRenderer);
        for (int i = 0; i < numVTiles * numHTiles; i++)
        {
            if (tiles[i].exists)
            {
                SDL_Rect dr = {.x = tiles[i].x * tileSize, .y = tiles[i].y * tileSize, .w = tileSize, .h = tileSize};
                SDL_Rect sr = {.x = tiles[i].tile.x, .y = tiles[i].tile.y, .w = tileSize, .h = tileSize};

                SDL_RenderCopy(editRenderer, tileSets[currentTileSet][TR_EDIT], &sr, &dr);
            }
        }
        
        if (SDL_PollEvent(&event))
        {
//            printf("Got an event\n");
            switch (event.type)
            {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                {
                    printf("quitting\n");
                    finishup();
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                printf("mouse clicked\n");
                if (SDL_GetWindowFromID(event.button.windowID) == mainWindow)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        selected.x = (event.button.x / tileSize) * tileSize;
                        selected.y = (event.button.y / tileSize) * tileSize;
                        printf("Selected: %d %d\n", selected.x, selected.y);
                    }
                }
                else
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        int xt = event.button.x / tileSize;
                        int yt = event.button.y / tileSize;
                        struct MapTile tmp = {.x = xt, .y = yt, .tile = {.x = selected.x, .y = selected.y}, .exists = true};
                        int possiblePlace = -1;
                        for (int i = 0; i < numHTiles * numVTiles; i++)
                        {

                            if (possiblePlace == -1 && !tiles[i].exists)
                                possiblePlace = i;
                            
                            if ((tiles[i].x == tmp.x && tiles[i].y == tmp.y))
                            {
                                printf("Inserting tile at %d\n", i);
                                printf("position: %d %d\n", tmp.x, tmp.y);
                                possiblePlace = i;

                                break;
                            }
                        }
                        json_t *tileAt = json_array_get(jsonTiles, yt * numHTiles + xt);
                        json_t *jtmp = json_object();
                        json_object_set_new(jtmp, "tileX", json_integer(selected.x / tileSize));
                        json_object_set_new(jtmp, "tileY", json_integer(selected.y / tileSize));
                        printf("sy, sx: %d %d\nxt, yt: %d, %d\n", selected.y, selected.x, xt, yt);
                        json_array_set_new(jsonTiles, yt * numHTiles + xt, jtmp);
                        
                        tiles[possiblePlace] = tmp;
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                    {
                        int xt = event.motion.x / tileSize;
                        int yt = event.motion.y / tileSize;
                        for (int i = 0; i < numHTiles * numVTiles; i++)
                        {
                            if ((tiles[i].x == xt && tiles[i].y == yt))
                            {
                                tiles[i].exists = false;

                                break;
                            }
                        }
                        if (json_array_get(jsonTiles, yt * numHTiles + xt) != json_null())
                        {
                            json_array_set_new(jsonTiles, yt * numHTiles + xt, json_null());
                        }
                    }         
                }
                break;

            case SDL_MOUSEMOTION:
                if (SDL_GetWindowFromID(event.button.windowID) == editWindow)
                {
                    if ((event.motion.state | (~SDL_BUTTON_LMASK)) ^ (~SDL_BUTTON_LMASK))
                    {
                        int xt = event.motion.x / tileSize;
                        int yt = event.motion.y / tileSize;
                        struct MapTile tmp = {.x = xt, .y = yt, .tile = {.x = selected.x, .y = selected.y}, .exists = true};
                        int possiblePlace = -1;
                        for (int i = 0; i < numHTiles * numVTiles; i++)
                        {

                            if (possiblePlace == -1 && !tiles[i].exists)
                                possiblePlace = i;
                            
                            if ((tiles[i].x == tmp.x && tiles[i].y == tmp.y))
                            {
                                printf("Inserting tile at %d\n", i);
                                printf("position: %d %d\n", tmp.x, tmp.y);
                                possiblePlace = i;

                                break;
                            }
                        }
                        json_t *jtmp = json_object();
                        json_object_set_new(jtmp, "tileX", json_integer(selected.x / tileSize));
                        json_object_set_new(jtmp, "tileY", json_integer(selected.y / tileSize));
                        printf("sy, sx: %d %d\nxt, yt: %d, %d\n", selected.y, selected.x, xt, yt);
                        json_array_set_new(jsonTiles, yt * numHTiles + xt, jtmp);
                        tiles[possiblePlace] = tmp;
                    }
                    else if ((event.motion.state | (~SDL_BUTTON_RMASK)) ^ (~SDL_BUTTON_RMASK))
                    {
                        int xt = event.motion.x / tileSize;
                        int yt = event.motion.y / tileSize;
                        for (int i = 0; i < numHTiles * numVTiles; i++)
                        {
                            if ((tiles[i].x == xt && tiles[i].y == yt))
                            {
                                tiles[i].exists = false;

                                break;
                            }
                        }
                        if (json_array_get(jsonTiles, yt * numHTiles + xt) != json_null())
                        {
                            json_array_set_new(jsonTiles, yt * numHTiles + xt, json_null());
                        }
                    }
                }
                break;                                        
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_s)
                    save();
                else if (event.key.keysym.sym == SDLK_f)
                {
                    fill(tiles);
                }
                else if (event.key.keysym.sym == SDLK_c)
                {
                    clear(tiles);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_RenderPresent(editRenderer);
    }

}
