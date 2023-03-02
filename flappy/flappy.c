#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SDL_DISABLE_IMMINTRIN_H
#include <SDL.h>
#include <SDL_ttf.h>

#define W 480 // Ширина экрана
#define H 600 // Высота экрана
#define GROUND 80 // Высота земли
#define PIPE_W 86 // Ширина труб
#define PHYS_W (W + PIPE_W + 80) // Общая ширина мира игры
#define GAP 220 // Расстояние между верхней и нижней трубами
#define GRACE 4 // Расстояние, на которое чар может подлететь к трубам, не столкнувшись с ними
#define RANDOM_PIPE_HEIGHT (rand() % (H - GROUND - GAP - 120) + 60) // Макрос для генерации случайной высоты трубы
#define PLYR_X 80 // Горизонтальное положение чара
#define PLYR_SZ 60 // Размер спрайта чара

enum gamestates {READY, ALIVE, GAMEOVER} gamestate = READY; // enum для состояний игры

float player_y = (H - GROUND)/2; // Вертикальное положение игрока
float player_vel; // Вертикальная скорость игрока
int pipe_x[2] = {W, W}; // Горизонтальное положение двух труб
float pipe_y[2]; // Вертикальное положение двух труб
int score; // Текущий счет
int best; // Лучший счет
int idle_time = 30; // Время, прошедшее с окончания игры (будет использоваться для time counter)
float frame = 0; // Номер кадра для анимации чара

SDL_Event event;
SDL_Renderer *renderer; // SDL Рендер context
SDL_Surface *surf; // Состояния
SDL_Texture *pillar; // Текстура труб
SDL_Texture *background; // Текстура фона
SDL_Texture *character[4]; // Текстура состояний чара (4)
TTF_Font *font; // Font для рендеринга текста

void setup(); // Функция инициализации SDL и настройки игры
void new_game(); // Функция для начала новой игры
void update_stuff(); // Функция для обновления состояния игры
void update_pipe(int i); // Функция для обновления одной трубы
void draw_stuff(); // Функция для отрисовки всего на экране
void text(char *fstr, int value, int height); // Функция для отображения текста

// Основной игровой цикл loop
int main()
{
        setup();

        for(;;) // Бесконечный цикл для игровой логики и рендеринга
        {
            while(SDL_PollEvent(&event)) switch(event.type) // Цикл для обработки ввода пользователя
                {
                    case SDL_QUIT: // Событие выхода из игры (закрытие окна)
                        exit(0);
                    case SDL_KEYDOWN: // Событие нажатия клавишу
                    case SDL_MOUSEBUTTONDOWN: // Событие нажатия мышь
                        if(gamestate == ALIVE) // Пропускаем, если игра уже идет
                        {
                            player_vel = -11.7f; // Задаем скорость прыжка игрока
                            frame += 1.0f; // Инкрементируем кадр анимации
                        }
                        else if(idle_time > 30) // Начинаем новую игру, если неактивны более 30 кадров
                        {
                            new_game();
                        }
                }

                update_stuff(); // Обновляем состояние игры
                draw_stuff(); // Рисуем всё на экране
                SDL_Delay(1000 / 60); // Задержка для ограничения частоты кадров до 60 в секунду
                idle_time++; // Инкрементируем idle time counter
        }
}

// Начальная настройка окна и рендеринга
void setup()
{
        srand(time(NULL)); // Задаём начальное значение Srand(time.h) на основе текущего времени

        SDL_Init(SDL_INIT_VIDEO); // Инициализируем SDL video подсистему
        SDL_Window *win = SDL_CreateWindow("Titaevskiy", // Создаём окно
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC); // Создание рендера для рисования в окне
        // Выходим, если не удалось создать рендерер
        if(!renderer) exit(fprintf(stderr, "Could not create SDL renderer\n"));

        surf = SDL_LoadBMP("res/pillar.bmp"); // Загружаем изображения столбов и фона и бэкграунда, создаём из них текстуры
        SDL_SetColorKey(surf, 1, 0xffff00);
        pillar = SDL_CreateTextureFromSurface(renderer, surf);
        surf = SDL_LoadBMP("res/background.bmp");
        background = SDL_CreateTextureFromSurface(renderer, surf);

        // Загружаем изображения чара и создаём из них текстуры
        for(int i = 0; i < 4; i++)
            {
                    char file[80];
                    sprintf(file, "res/character-%d.bmp", i);
                    surf = SDL_LoadBMP(file);
                    SDL_SetColorKey(surf, 1, 0xffff00);
                    character[i] = SDL_CreateTextureFromSurface(renderer, surf);
            }

    TTF_Init(); // инициализирование библиотеки TTF для загрузки шрифта(для текстов)
    font = TTF_OpenFont("res/LiberationMono-Regular.ttf", 42);
}

// Начать новую игру
void new_game()
{
        gamestate = ALIVE;
        player_y = (H - GROUND)/2;
        player_vel = -11.7f;
        score = 0;
        pipe_x[0] = PHYS_W + PHYS_W/2 - PIPE_W;
        pipe_x[1] = PHYS_W - PIPE_W;
        pipe_y[0] = RANDOM_PIPE_HEIGHT;
        pipe_y[1] = RANDOM_PIPE_HEIGHT;
}

// При ударе
void game_over()
{
        gamestate = GAMEOVER;
        idle_time = 0;
        if(best < score) best = score;
}

// Добавить обновление состояний игры (позицию чара, позицию труб, счет)
// Обновляем все, что должно обновляться само по себе, без ввода
void update_stuff()
{
        if(gamestate != ALIVE) return;

        player_y += player_vel;
        player_vel += 0.61; // гравитация

        if(player_vel > 10.0f)
                frame = 0;
        else
                frame -= (player_vel - 10.0f) * 0.03f; // модная анимация

        if(player_y > H - GROUND - PLYR_SZ)
                game_over();

        for(int i = 0; i < 2; i++)
                update_pipe(i);
}

// обновить одну трубу для одного кадра(положение)
void update_pipe(int i)
{
        if(PLYR_X + PLYR_SZ >= pipe_x[i] + GRACE && PLYR_X <= pipe_x[i] + PIPE_W - GRACE && // Если игрок столкнется с трубой, конец игры
            (player_y <= pipe_y[i] - GRACE || player_y + PLYR_SZ >= pipe_y[i] + GAP + GRACE))
            game_over(); // игрок ударился о трубу

        // Перемещение труб и увеличение счета игрока, если прошли 1 трубу
        pipe_x[i] -= 5;
        if(pipe_x[i] + PIPE_W < PLYR_X && pipe_x[i] + PIPE_W > PLYR_X - 5)
                score++;

        // стартовая труда за пределами экрана
        if(pipe_x[i] <= -PIPE_W)
        {
                pipe_x[i] = PHYS_W - PIPE_W;
                pipe_y[i] = RANDOM_PIPE_HEIGHT;
        }
}

// отрисовывает все эдементы на экране
void draw_stuff()
{
        SDL_Rect dest = {0, 0, W, H}; // Создаем прямоугольник, который каверит весь экран
        SDL_RenderCopy(renderer, background, NULL, &dest); // Копируем бэкграунд на рендер и каверим им весь экран

        // отображаем трубы
        for(int i = 0; i < 2; i++)
        {
                int lower = pipe_y[i] + GAP; // Вычисляем нижний край верхней трубы, добавив промежуток между трубами
                SDL_RenderCopy(renderer, pillar, NULL, &(SDL_Rect){pipe_x[i], pipe_y[i] - H, PIPE_W, H}); // Отображаем верхнюю трубу на экране
                SDL_Rect src = {0, 0, 86, H - lower - GROUND}; // (Сложная ХУЙНЯ) Создаем прямоугольник, который представляет собой часть нижней трубы, которая будет видна на экране
                SDL_RenderCopy(renderer, pillar, &src, &(SDL_Rect){pipe_x[i], lower, PIPE_W, src.h}); // Отображаем нижнюю трубу на экране
        }

        // отображаем игрока
        SDL_RenderCopy(renderer, character[(int)frame % 4], NULL,
                        &(SDL_Rect){PLYR_X, player_y, PLYR_SZ, PLYR_SZ});

        // Показываем счет или сообщение в зависимости от состояния игры
        if(gamestate != READY) text("%d", score, 10);
        if(gamestate == READY) text("Press any key", 0, 150);
        if(gamestate == GAMEOVER) text("High score: %d", best, 150);

        SDL_RenderPresent(renderer); // Обновите экран со всеми нарисованными элементами
}

// Эта функция выводит текстовое сообщение на экран
void text(char *fstr, int value, int height)
{
        if(!font) return; // Вернуть, если шрифт недоступен
        int w, h; // Переменные для хранения ширины и высоты текстового сообщения
        // Создает строку с текстовым сообщением и значением (если оно есть)
        char msg[80];
        snprintf(msg, 80, fstr, value);
        TTF_SizeText(font, msg, &w, &h); // Получить размер текстового сообщения
        SDL_Surface *msgsurf = TTF_RenderText_Blended(font, msg, (SDL_Color){255, 255, 255}); // Отобразить текстовое сообщение на поверхности белого цвета
        SDL_Texture *msgtex = SDL_CreateTextureFromSurface(renderer, msgsurf); // Создание текстуры from surface
        SDL_Rect fromrec = {0, 0, msgsurf->w, msgsurf->h}; // Создаем прямоугольник, который каверит всю поверхность
        SDL_Rect torec = {(W - w)/2, height, msgsurf->w, msgsurf->h}; // Создаем прямоугольник, который определяет, где на экране будет отображаться текстовое сообщение
        SDL_RenderCopy(renderer, msgtex, &fromrec, &torec); // Копируем текстуру в средство визуализации, отобразив текстовое сообщение на экране
        // Уничтожаем текстуру, освобождаем поверхность, чтобы избежать утечек памяти
        SDL_DestroyTexture(msgtex);
        SDL_FreeSurface(msgsurf);
}
