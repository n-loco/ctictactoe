/**
 * Autor - Nícolas "N_Loco" Rickes dos Santos  (2025)
 * https://github.com/n-loco/
 *
 * **LEIA:**
 * Este programa é um Jogo da Velha de terminal
 * feito puramente em C99, POSIX e Win32.
 *
 * A ideia dele é ser estudável por outras pessoas,
 * em especial, meus amigos e colegas interessados,
 * então espere comentários em português e muitas
 * vezes bem básicos.
 *
 * Você pode ler e estudar como quiser, pedir
 * para LLMs explicarem partes ou tudo (mas se
 * dê o trabalho de entender 😠) e eu recomendo
 * aqui alguns links que considero úteis:
 *  - https://xumaquer.github.io/mdbook-linguagem-c/ (feito por um amigo)
 *  - https://en.cppreference.com/w/c.html
 *
 * O programa é dedicado ao *domínio público*, e
 * por garantia, atribuo esta licença: https://unlicense.org/,
 * então você pode copiar, modificar, redistribuir
 * e qualquer outra coisa.
 */

// Necessário para usar alguns recursos definidos pelo padrão POSIX,
// seguido por sistemas Unix-like como Linux, macOS, etc.
#if defined (__unix__) || defined (__APPLE__)
# define _POSIX_C_SOURCE 199309L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

/**
 * Cabeçalhos específicos de cada sistema, isso
 * é necessário, pois para algumas coisas eu
 * preciso de funções extras que não existem
 * em C 100% puro, e então uso funções
 * do próprio sistema operacional.
 */
#if defined (_WIN32)
# include <windows.h>
#elif defined (__unix__) || defined (__APPLE__)
# include <unistd.h>
# include <termios.h>
# include <sys/ioctl.h>
#endif

/**
 * `1b` em hexadecimal, representa
 * a tecla ESC (Escape) do teclado.
 * 
 * Ase squências usadas nesse programa
 * que começam com ESC, são um padrão ANSI,
 * e servem para fazer coisas como
 * mudar cores, formatação, mover o cursor,
 * limpar a tela, etc.
 * Então sempre que ver esse `ESC` no começo
 * seguido por uma sequência de caracteres,
 * saiba que é uma sequência ANSI.
 * 
 * Para mais informações, você pode visitar:
 *  - https://xumaquer.github.io/mdbook-linguagem-c/x-03-terminal.html
 *  - https://vt100.net/
 *  - https://en.wikipedia.org/wiki/ANSI_escape_code
 *  - https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
 */
#define ESC "\x1b"

/**
 * Interrompe a execução do
 * programa por `ms` milissegundos.
 */
void block_delay(uint32_t ms)
{
#if defined (_WIN32)
    Sleep(ms);
#elif defined (__unix__) || defined (__APPLE__)
    struct timespec t = {0};
    if (ms >= 1000)
    {
        t.tv_sec = ms / 1000;
        t.tv_nsec = (ms % 1000) * 1000000;
    }
    else
        t.tv_nsec = ms * 1000000;
    nanosleep(&t, NULL);
#endif
}

/**
 * Vetor 2D.
 */
struct Vec2
{
    int32_t x;
    int32_t y;
};

/**
 * Macro ajudante para vetores inline.
 */
#define vec2(x, y) ((struct Vec2){x, y})

/**
 * Cor RGB.
 */
struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

/**
 * Macro ajudante para cores inline.
 */
#define rgb(r, g, b) ((struct Color){r, g, b})

/**
 * Pega o tamanho do terminal.
 */
struct Vec2 display_size()
{
#if defined (_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO ws = {0};
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ws);
    return vec2(
        (ws.srWindow.Right - ws.srWindow.Left) + 1,
        (ws.srWindow.Bottom - ws.srWindow.Top) + 1
    );
#elif defined (__unix__) || defined (__APPLE__)
    struct winsize ws = {0};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return vec2(ws.ws_col, ws.ws_row);
#endif
}

/**
 * Reseta todas as formatações.
 */
void reset_formatting()
{
    printf(ESC"[0m");
}

/**
 * Entra no modo de escrita em negrito.
 */
void set_bold()
{
    printf(ESC"[1m");
}

/**
 * Entra no modo de escrita esvaecida.
 */
void set_dim()
{
    printf(ESC"[2m");
}

/**
 * Entra no modo de escrita em itálico.
 */
void set_italic()
{
    printf(ESC"[3m");
}

/**
 * Muda a cor de escrita do terminal.
 */
void set_foreground_color(struct Color color)
{
    printf(ESC"[38;2;%u;%u;%um", color.r, color.g, color.b);
}

/**
 * Muda a cor de fundo do terminal.
 */
void set_background_color(struct Color color)
{
    printf(ESC"[48;2;%u;%u;%um", color.r, color.g, color.b);
}

/**
 * Move o cursor relativamente à
 * sua posição atual.
 */
void move_cursor(struct Vec2 pos)
{
    if (pos.x != 0)
    {
        if (pos.x < 0)
            printf(ESC"[%dD", pos.x * -1);
        else
            printf(ESC"[%dC", pos.x);
    }

    if (pos.y != 0)
    {
        if (pos.y < 0)
            printf(ESC"[%dA", pos.y * -1);
        else
            printf(ESC"[%dB", pos.y);
    }
}

/**
 * Define a posição exata do cursor.
 *
 * NOTA: Valores negativos NÃO funcionam.
 */
void set_cursor_position(struct Vec2 pos)
{
    printf(ESC"[%d;%dH", pos.y, pos.x);
}

/**
 * Põe o cursor no começo do terminal.
 */
void rewind_cursor()
{
    set_cursor_position(vec2(0, 0));
}

/**
 * Sempre põe o cursor no começo do terminal.
 *
 * Caso `force_clean` seja `true`, todo o terminal
 * é limpo, caso seja `false`, o terminal só
 * será limpo se o terminal mudou te tamanho.
 */
void new_screen_frame(bool force_clean)
{
    // A função "lembra" do tamanho
    // anterior do terminal.
    static struct Vec2 prev_size = {0};

    rewind_cursor();

    struct Vec2 size = display_size();

    bool size_changed = (prev_size.x != size.x) || (prev_size.y != size.y);
    
    if (force_clean || size_changed)
        printf(ESC"[J");

    if (size_changed)
        prev_size = size;
}

/**
 * Estrutura que guarda
 * configurações do terminal.
 */
struct TerminalConfig
{
#if defined (_WIN32)
    UINT __output_codepage;
    UINT __codepage;
    DWORD __output_cfg;
    DWORD __input_cfg;
#elif defined (__unix__) || defined (__APPLE__)
    struct termios __input_cfg;
#endif
};

/**
 * Informações originais do terminal.
 */
struct TerminalConfig original_terminal_cfg = {0};

/**
 * Restaura as configurações originais do terminal.
 */
void restore_terminal(void)
{
    // Volta a mostrar o cursor.
    printf(ESC"[?25h");
    // Volta para o buffer principal.
    printf(ESC"[?1049l");

    // Aplicando as configurações originais do terminal.
#if defined (_WIN32)
    SetConsoleCP(original_terminal_cfg.__codepage);
    SetConsoleOutputCP(original_terminal_cfg.__output_codepage);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), original_terminal_cfg.__input_cfg);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), original_terminal_cfg.__output_cfg);
#elif defined (__unix__) || defined (__APPLE__)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_cfg.__input_cfg);
#endif
}

/**
 * Em ambos os sistemas:
 *  Configura a entrada do terminal
 *  para ficar no modo "cru", muda
 *  para o buffer alternativo do terminal
 *  e coloca o buffer da `stdout` como `_IONBF` (sem buffer)
 *
 * Somente no Windows:
 *  Configura a saída para ficar em utf-8, além
 *  de configurar o terminal para aceitar
 *  codes (as sequências que começam com ESC).
 */
void setup_terminal()
{
    // Aqui já precisamos fazer configurações
    // específicas para cada sistema.
#if defined (_WIN32)
    original_terminal_cfg.__output_codepage = GetConsoleOutputCP();
    original_terminal_cfg.__codepage = GetConsoleCP();

    // Dizendo ao terminal para utilizar utf-8
    // para não nos preocuparmos com caracteres especiais.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE stdin_h = GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode(stdout_h, &original_terminal_cfg.__output_cfg);
    GetConsoleMode(stdin_h, &original_terminal_cfg.__input_cfg);

    DWORD stdout_mode = original_terminal_cfg.__output_cfg;
    DWORD stdin_mode = original_terminal_cfg.__input_cfg;

    // Precisamos informar ao terminal que
    // queremos ser capazes de utilizar as sequências ANSI
    // (as que começam com ESC).
    stdout_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // Assim como queremos receber sequências ANSI como entrata também.
    stdin_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    // Finalmente, colocamos a entrada no "modo cru".
    stdin_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

    SetConsoleMode(stdout_h, stdout_mode);
    SetConsoleMode(stdin_h, stdin_mode);
#elif defined (__unix__) || defined (__APPLE__)
    tcgetattr(STDIN_FILENO, &original_terminal_cfg.__input_cfg);

    struct termios stdin_trms = original_terminal_cfg.__input_cfg;
    // Colocando a entrada no "modo cru".
    stdin_trms.c_lflag &= ~(ECHO | ICANON | ISIG);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_trms);
#endif
    // Só por garantia...
    setlocale(LC_ALL, "pt_BR.UTF-8");
    
    // Tiramos a bufferização da saída
    // para não precisarmos ter que usar `\n`
    // e nem usar `fflush`.
    // Não é bom para performance
    // mas deixa as coisas mais simples.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Muda para o buffer alternativo.
    printf(ESC"[?1049h");
    // Esconde o cursor.
    printf(ESC"[?25l");

    rewind_cursor();

    // Quando o programa fechar
    // `restore_terminal` vai ser chamado
    // e fazer o terminal voltar ao
    // normal.
    atexit(restore_terminal);
}

/**
 * Lê dados brutos direto da
 * entrada do terminal e escreve
 * em `seq` de tamanho `n`.
 *
 * Retorna a quantidade de bytes
 * que leu.
 */
size_t raw_input(uint8_t *seq, size_t n)
{
#if defined (_WIN32)
    DWORD rd;
    ReadFile(GetStdHandle(STD_INPUT_HANDLE), seq, n, &rd, NULL);
    return rd;
#elif defined (__unix__) || defined (__APPLE__)
    return read(STDIN_FILENO, seq, n);
#endif
}

/**
 * Inputs processados do teclado.
 *
 * NOTA: Não coloquei TODAS as teclas,
 * coloquei apenas o que é conveniente.
 */
enum KeyboardInput
{
    KEY_UNSUPPORTED,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_A,
    KEY_D,
    KEY_Q,
    KEY_S,
    KEY_W,
    KEY_SPACE,
    KEY_BACKSPACE,
    KEY_ESCAPE,
    KEY_ENTER,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_ARROW_RIGHT,
    KEY_ARROW_LEFT,
};

/**
 * Função que bloqueia a execução,
 * espera uma entrada do usuário e
 * devolve a tecla representada em
 * `KeyboardInput`.
 */
enum KeyboardInput keyboard_input()
{
    uint8_t seq[9] = {0};
    size_t seq_n = raw_input(seq, 8);

    if (seq_n == 1)
        switch (seq[0])
        {
        case '1': return KEY_1;
        case '2': return KEY_2;
        case '3': return KEY_3;
        case 'a': return KEY_A;
        case 'd': return KEY_D;
        case 'q': return KEY_Q;
        case 's': return KEY_S;
        case 'w': return KEY_W;
        case ' ': return KEY_SPACE;
        case 0x7f: return KEY_BACKSPACE;
        case 0x1b: return KEY_ESCAPE;
        case '\r': case '\n': return KEY_ENTER;
        default: return KEY_UNSUPPORTED;
        }
    if (seq_n == 3 && seq[0] == 0x1b && seq[1] == '[' && ('A' <= seq[2] && seq[2] <= 'D'))
        switch (seq[2])
        {
        case 'A': return KEY_ARROW_UP;
        case 'B': return KEY_ARROW_DOWN;
        case 'C': return KEY_ARROW_RIGHT;
        case 'D': return KEY_ARROW_LEFT;
        }
    return KEY_UNSUPPORTED;
}

/**
 * Define a jogada feita.
 */
enum Move
{
    FREE_MOVE,
    X_MOVE,
    O_MOVE,
};

/**
 * O tabuleiro do jogo.
 */
typedef enum Move GameBoard[3][3];

/**
 * Muda uma jogada de uma célula do tabuleiro.
 */
static inline void set_game_board_cell(GameBoard board, struct Vec2 pos, enum Move move)
{
    board[pos.y][pos.x] = move;
}

/**
 * Pega a jogada de uma célula do tabuleiro.
 */
static inline enum Move game_board_cell(GameBoard board, struct Vec2 pos)
{
    return board[pos.y][pos.x];
}

/**
 * É só um inteiro, ele funciona
 * como uma "matriz de booleanos"
 * para guardar as jogadas de um ator
 * e fazer comparações mais rápido.
 * 
 * NOTA: Apenas os 9 bits menos
 * significativos são realmente
 * importantes.
 */
typedef uint16_t MovePrint;

/**
 * Todas as combinações vencedoras.
 *
 * 0-2 => horizontais;
 * 3-5 => verticais;
 * 6 7 => diagonais.
 */
const MovePrint match_move_prints[] = {
    // Os números estão em octal.
    // Horizontais.
    0007, 0070, 0700,
    // Verticais.
    0111, 0222, 0444,
    // Diagnoais.
    0421, 0124,
};

/**
 * Troca um bit do `MovePrint`.
 */
static inline void edit_move_print(MovePrint *print, struct Vec2 pos, bool bit)
{
    uint16_t mask = (1 << ((pos.y * 3) + pos.x));
    if (bit)
        *print |= mask;
    else
        *print &= ~mask;
}

/**
 * Pega um bit do `MovePrint`.
 */
static inline bool move_print_inspec(MovePrint print, struct Vec2 pos)
{
    return (print & (1 << ((pos.y * 3) + pos.x))) > 0;
}

/**
 * Estrutura que guarda jogadas
 * separadamente em 3 `MovePrint`s
 * para análise mais fácil.
 */
struct MovePrintTriplet
{
    MovePrint free;
    MovePrint x;
    MovePrint o;
};

/**
 * Gera um `MovePrintTriplet` a
 * partir do tabuleiro do jogo.
 */
struct MovePrintTriplet get_move_print_triplet(GameBoard board)
{
    struct MovePrintTriplet triplet = {0};
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            MovePrint *print = NULL;
            enum Move move = game_board_cell(board, vec2(x, y));
            switch (move)
            {
            case FREE_MOVE: print = &triplet.free; break;
            case X_MOVE: print = &triplet.x; break;
            case O_MOVE: print = &triplet.o; break;
            }
            edit_move_print(print, vec2(x, y), true);
        }
    }
    return triplet;
}

/**
 * Retorna a quantidade de
 * jogadas em um `MovePrint`.
 */
static inline uint8_t move_print_count(MovePrint print)
{
    uint8_t count = 0;
    for (int i = 0; i < 9; i++)
        count += (print >> i) & 1;
    return count;
}

/**
 * Escreve as coordenadas de cada bit
 * em `print` dentro de `coords`.
 *
 * Use `move_print_count` para saber
 * o atamanho do array.
 */
void move_print_coords(MovePrint print, struct Vec2 coords[])
{
    int coord_i = 0;
    for (int i = 0; i < 9; i++)
        if ((print >> i) & 1)
        {
            coords[coord_i] = vec2(i % 3, i / 3);
            coord_i++;
        }
}

/**
 * Compara 2 `MovePrint`s, retornando
 * `true` se a linha estiver pura e
 * e `false` se ela não estiver pura.
 *
 * A pureza da linha é em relação
 * ao `testing`, se houver uma jogada
 * do `opponent` na linha, ela é considerada
 * impura, se só houver jogadas do `testing`
 * ela é pura.
 */
static inline bool test_move_print_purity(MovePrint testing, MovePrint opponent)
{
    return (testing | opponent) == testing;
}

/**
 * Testa se há uma combinação
 * vencedora em uma linha e retorna
 * o `MovePrint`.
 */
static inline MovePrint test_move_print_winner_line(MovePrint testing, size_t match_index)
{
    if ((testing & match_move_prints[match_index]) == match_move_prints[match_index])
        return match_move_prints[match_index];
    return 0;
}

/**
 * Pega todas as jogadas consideradas
 * vencedoras e as retorna como `MovePrint`.
 */
MovePrint test_move_print_winner(MovePrint testing)
{
    MovePrint result = 0;
    for (int i = 0; i < 8; i++)
        result |= test_move_print_winner_line(testing, i);
    return result;
}

/**
 * Representa o símbolo que vai jogar.
 */
enum Actor
{
    NULL_ACTOR,
    X_ACTOR,
    O_ACTOR,
};

/**
 * Converte de `Actor` para `Move`.
 */
#define actor_to_move(a) ((enum Move)a)

/**
 * Retorna o `Actor` oponente.
 */
#define opponent_actor(a) ((a == NULL_ACTOR) ? NULL_ACTOR : ((a == X_ACTOR) ? O_ACTOR : X_ACTOR))

/**
 * A cor que não representa
 * nenhum jogador :^)
 */
#define NULL_ACTOR_COLOR rgb(178, 82, 218);

/**
 * Pega a cor que representa o símbolo.
 */
struct Color actor_color(enum Actor actor)
{
    switch (actor)
    {
    case X_ACTOR:
        return rgb(255, 51, 136);
    case O_ACTOR:
        return rgb(64, 204, 255);
    case NULL_ACTOR:
        return NULL_ACTOR_COLOR;
    }
}

/**
 * Representa a continuidade
 * ou final do jogo.
 */
enum EndGame
{
    RUNNING,
    GAME_DRAW,
    X_VICTORY,
    O_VICTORY,
};

/**
 * Pega a cor do emapte (velha).
 */
struct Color game_draw_color()
{
    return NULL_ACTOR_COLOR;
}

/**
 * Representa todo o estado do jogo.
 */
struct GameState
{
    /**
     * A grade 3x3 do jogo.
     */
    GameBoard board;
    /**
     * A seleção atual do jogador (ou IA).
     */
    struct Vec2 selection;
    /**
     * Quem vai jogar.
     */
    enum Actor turn;
    /**
     * Quantidade de jogadas.
     */
    uint8_t moves;
    /**
     * Continuidade do jogo.
     */
    enum EndGame endgame;
};

/**
 * Desenha o símbolo representando o ator
 * usando a sua cor respectiva.
 */
void draw_game_actor(enum Actor actor)
{
    if (actor != NULL_ACTOR)
        set_foreground_color(actor_color(actor));

    switch (actor)
    {
    case O_ACTOR:
        putchar('O');
        break;
    case X_ACTOR:
        putchar('X');
        break;
    case NULL_ACTOR:
        putchar(' ');
        break;
    }
    if (actor != NULL_ACTOR)
        reset_formatting();
}

/**
 * Desenha o quadro em volta das células do tabuleiro.
 */
void render_game_frame(enum EndGame endgame)
{
    set_bold();

    if (endgame == GAME_DRAW)
        set_foreground_color(game_draw_color());
    else if (endgame == X_VICTORY)
        set_foreground_color(actor_color(X_ACTOR));
    else if (endgame == O_VICTORY)
        set_foreground_color(actor_color(O_ACTOR));

    printf("╭─ C Tic Tac Toe ─╮");
    move_cursor(vec2(-19, 1));

    for (int i = 0 ; i < 9; i++)
    {
        printf("│");
        move_cursor(vec2(17, 0));
        printf("│");
        move_cursor(vec2(-19, 1));
    }

    printf("╰─────────────────╯");
    reset_formatting();
}

/**
 * Desenha a continuidade do jogo.
 */
void render_endgame(enum EndGame endgame, enum Actor turn, int moves)
{
    switch (endgame)
    {
    case GAME_DRAW:
        set_background_color(game_draw_color());
        printf("    Deu velha!     ");
        break;
    case O_VICTORY:
        set_background_color(actor_color(O_ACTOR));
        printf("  O é o vencedor!  ");
        break;
    case X_VICTORY:
        set_background_color(actor_color(X_ACTOR));
        printf("  X é o vencedor!  ");
        break;
    case RUNNING:
        printf("     Turno: ");
        draw_game_actor(turn);
        move_cursor(vec2(-13, 1));
        reset_formatting();
        return;
    }
    reset_formatting();
    move_cursor(vec2(-19, 1));
}

/**
 * Desenha a jogada.
 */
void draw_game_move(enum Move move)
{
    switch (move)
    {
    case FREE_MOVE:
        draw_game_actor(NULL_ACTOR);
        break;
    case X_MOVE:
        draw_game_actor(X_ACTOR);
        break;
    case O_MOVE:
        draw_game_actor(O_ACTOR);
        break;
    }
}

/**
 * Desenha o quadro da célula da grade.
 */
void draw_game_cell_frame(enum Actor actor, bool highlighted)
{
    if (highlighted)
    {
        set_bold();
        set_foreground_color(actor_color(actor));
    }
    else
        set_dim();

    printf("╭───╮");
    move_cursor(vec2(-5, 1));

    printf("│");
    move_cursor(vec2(3, 0));
    printf("│");
    move_cursor(vec2(-5, 1));

    printf("╰───╯");
    move_cursor(vec2(-5, -2));

    reset_formatting();
}

/**
 * Desenha uma célula individual
 * da grade do jogo.
 */
void draw_game_cell(enum Actor actor, enum Move move, bool highlighted)
{
    draw_game_cell_frame(actor, highlighted);
    move_cursor(vec2(2, 1));
    draw_game_move(move);
    move_cursor(vec2(-3, -1));
}

/**
 * Desenha o tabuleiro do jogo,
 * mais precisamente, as células.
 *
 * `highlight` escolhe a cor baseado
 * no `actor`.
 */
void render_game_board(GameBoard board, enum Actor actor, MovePrint highlight)
{
    for (int y = 0; y < 3; y++)
    {
        int moved_x = 0;

        for (int x = 0; x < 3; x++)
        {
            struct Vec2 pos = {x, y};

            bool highlighted = move_print_inspec(highlight, pos);
            enum Move move_in_cell = game_board_cell(board, pos);

            draw_game_cell(actor, move_in_cell, highlighted);

            moved_x += 5;
            move_cursor(vec2(5, 0));
        }

        move_cursor(vec2(-moved_x, 3));
    }
}

/**
 * É como `render_game_board`, mas possui
 * uma animação "pintando" as células.
 */
void animate_board_rendering(GameBoard board, enum Actor actor, MovePrint highlight)
{
    int highlighted_len = move_print_count(highlight);
    struct Vec2 highlighted_coords[9] = {0};
    move_print_coords(highlight, highlighted_coords);

    block_delay(100);

    MovePrint animated_highlight = 0;
    for (int i = 0; i <= highlighted_len; i++)
    {
        render_game_board(board, actor, animated_highlight);
        edit_move_print(&animated_highlight, highlighted_coords[i], true);

        if (i < highlighted_len)
        {
            move_cursor(vec2(0, -9));
            block_delay(50);
        }
    }
}

/**
 * Animação de preenchimento utilizada
 * quando o jogo da em velha.
 */
void game_board_fill_animation(GameBoard board, enum Actor turn, MovePrint missing)
{
    int missing_len = move_print_count(missing);
    struct Vec2 missing_coords[3] = {0};
    move_print_coords(missing, missing_coords);

    uint8_t animated = 0;

    render_game_board(board, NULL_ACTOR, 0);
    move_cursor(vec2(0, -9));

    block_delay(300);

    for (int i = 0; i < missing_len; i++)
    {
        int selection = 0; do
            selection = rand() % missing_len;
        while (animated & (1 << selection));

        set_game_board_cell(board, missing_coords[selection], actor_to_move(turn));
        turn = opponent_actor(turn);

        MovePrint selection_highlight = 0;
        edit_move_print(&selection_highlight, missing_coords[selection], true);

        render_game_board(board, NULL_ACTOR, selection_highlight);

        animated |= (1 << selection);
        if (i < missing_len)
            move_cursor(vec2(0, -9));
        block_delay(250);
    }
}

/**
 * Desenha o jogo.
 */
void render_game(struct GameState *state)
{
    struct Vec2 screen_size = display_size();
    struct Vec2 screen_offset = {(screen_size.x / 2) - 9, (screen_size.y / 2) - 6};
    new_screen_frame(false);
    set_cursor_position(screen_offset);

    render_game_frame(state->endgame);
    move_cursor(vec2(-19, 1));
    render_endgame(state->endgame, state->turn, state->moves);
    printf("    Jogadas: %d", state->moves);
    move_cursor(vec2(-12, -11));

    struct MovePrintTriplet separated_state = get_move_print_triplet(state->board);

    MovePrint highlighting = 0;
    switch (state->endgame)
    {
    case RUNNING:
        edit_move_print(&highlighting, state->selection, true);
        render_game_board(state->board, state->turn, highlighting);
        break;
    case GAME_DRAW:
        game_board_fill_animation(state->board, state->turn, separated_state.free);
        highlighting = separated_state.x | separated_state.o;
        animate_board_rendering(state->board, NULL_ACTOR, highlighting);
        break;
    case X_VICTORY:
        highlighting = test_move_print_winner(separated_state.x);
        animate_board_rendering(state->board, X_ACTOR, highlighting);
        break;
    case O_VICTORY:
        highlighting = test_move_print_winner(separated_state.o);
        animate_board_rendering(state->board, O_ACTOR, highlighting);
        break;
    }
}

/**
 * Representa a ação que o
 * jogador (ou IA) deseja fazer.
 */
enum GameInput {
    QUIT_INPUT,
    UP_INPUT,
    DOWN_INPUT,
    LEFT_INPUT,
    RIGHT_INPUT,
    MOVE_INPUT,
};

/**
 * Tipo genérico para argumentos
 * do GameInputSourceExecutor
 */
typedef void *GameInputSourceArgs;

/**
 * Tipo do formato da função que
 * pega um input de algum lugar.
 */
typedef enum GameInput(*GameInputSourceExecutor)(GameInputSourceArgs);

/**
 * Estrutura para pegar a
 * entrada de algo no jogo.
 */
struct GameInputSource
{
    GameInputSourceExecutor executor;
    GameInputSourceArgs args;
};

/**
 * Modifica o estado do jogo baseado
 * no input recebido pelo `game_input_source`.
 *
 * Sempre retorna `false`, ao menos que
 * o input recebido seja `QUIT_INPUT`, aí retorna `true`.
 */
bool process_game_input(struct GameState *state, struct GameInputSource game_input_source)
{
    enum GameInput input = game_input_source.executor(game_input_source.args);

    switch (input)
    {
    case QUIT_INPUT:
        return true;
    case UP_INPUT:
        state->selection.y = ((state->selection.y + 3) - 1) % 3;
        break;
    case DOWN_INPUT:
        state->selection.y = (state->selection.y + 1) % 3;
        break;
    case LEFT_INPUT:
        state->selection.x = ((state->selection.x + 3) - 1) % 3;
        break;
    case RIGHT_INPUT:
        state->selection.x = (state->selection.x + 1) % 3;
        break;
    case MOVE_INPUT:
    {
        struct Vec2 selection = state->selection;
        enum Move move_in_cell = game_board_cell(state->board, selection);
        if (move_in_cell == FREE_MOVE)
        {
            enum Move move = actor_to_move(state->turn);
            set_game_board_cell(state->board, selection, move);
            state->turn = opponent_actor(state->turn);
            state->moves++;
        }
    }
    }

    return false;
}

/**
 * Calcula o número mínimo de
 * jogadas necessárias para ganhar
 * até o final do jogo, levando
 * em consideração quem iniciou o
 * jogo.
 */
uint8_t calc_min_moves(bool starter, uint8_t moves)
{
    uint8_t moves_left = 0;
    if (starter)
        moves_left = 5 - (moves - (moves / 2));
    else
        moves_left = 4 - (moves / 2);

    uint8_t min_moves = 3 - moves_left;
    return min_moves;
}

/**
 * Detecta se houve algum
 * vencedor ou empate, e
 * então, atualiza o estado.
 */
void process_game_state(struct GameState *state)
{
    // Assim que o jogo começa, nós guardamos
    // quem começou, será SUPER importante
    // para o algoritmo de detectar velha.
    static enum Actor game_starter = NULL_ACTOR;
    if (state->moves == 0)
    {
        game_starter = state->turn;
        return;
    }

    // Não é possível ganhar um jogo com
    // 4 jogadas, então ¯\_(ツ)_/¯
    if (state->moves < 5)
        return;

    struct MovePrintTriplet separated_state = get_move_print_triplet(state->board);

    MovePrint x_moves = separated_state.x;
    MovePrint o_moves = separated_state.o;
    MovePrint free_moves = separated_state.free;

    MovePrint x_match = test_move_print_winner(x_moves);
    MovePrint o_match = test_move_print_winner(o_moves);
    if (x_match || o_match)
    {
        if (x_match)
            state->endgame = X_VICTORY;
        else
            state->endgame = O_VICTORY;
        return;
    }

    // Não há como detectar empate
    // com menos de 6 jogadas.
    if (state->moves < 6)
        return;

    bool is_x_starter = game_starter == X_ACTOR;
    bool is_o_starter = game_starter == O_ACTOR;

    uint8_t x_min_moves = calc_min_moves(is_x_starter, state->moves);
    uint8_t o_min_moves = calc_min_moves(is_o_starter, state->moves);

    size_t free_moves_len = move_print_count(free_moves);
    struct Vec2 free_moves_coords[3] = {0};
    move_print_coords(free_moves, free_moves_coords);


    for (int i = 0; i < free_moves_len; i++)
    {
        struct Vec2 cell_pos = free_moves_coords[i];

        MovePrint x_lines[] = {
            x_moves & match_move_prints[cell_pos.y],
            x_moves & match_move_prints[cell_pos.x + 3],
            x_moves & match_move_prints[6],
            x_moves & match_move_prints[7],
        };

        MovePrint o_lines[] = {
            o_moves & match_move_prints[cell_pos.y],
            o_moves & match_move_prints[cell_pos.x + 3],
            o_moves & match_move_prints[6],
            o_moves & match_move_prints[7],
        };

        for (int i = 0; i < 4; i++)
        {
            MovePrint x_line = x_lines[i];
            MovePrint o_line = o_lines[i];

            bool can_x_win = test_move_print_purity(x_line, o_line)
                && (move_print_count(x_line) >= x_min_moves);

            bool can_o_win = test_move_print_purity(o_line, x_line)
                && (move_print_count(o_line) >= o_min_moves);

            if (can_x_win || can_o_win)
                goto KEEP_GAME_RUNNING;
        }
    }

    state->endgame = GAME_DRAW;

    KEEP_GAME_RUNNING:
    return;
}

/**
 * Fica bloqueando o programa
 * até que o jogador digite alguma
 * tecla de confirmação ou saída/cancelamento.
 *
 * Retorna `true` para real confirmação
 * e retorna `false` para caso queira saír/cancelar.
 */
bool blocking_confirm()
{
    while (true)
    {
        enum KeyboardInput key = keyboard_input();

        switch (key)
        {
            case KEY_Q:
            case KEY_BACKSPACE:
            case KEY_ESCAPE:
                return false;
            case KEY_ENTER:
            case KEY_SPACE:
                return true;
            default:
                continue;
        }
    }
}

/**
 * Executa uma iteração de evento
 * no jogo, retorna `true` se deve
 * continuar, e `false` se deve parar.
 */
bool game_event_loop(struct GameState *state, struct GameInputSource game_input_source)
{
    process_game_state(state);

    render_game(state);

    if (state->endgame != RUNNING)
    {
        blocking_confirm();
        return false;
    }

    return !process_game_input(state, game_input_source);
}

/**
 * Bloqueia a execução e espera
 * uma entrada do jogador.
 *
 * Compatível com o tipo `GameInputSourceExecutor`.
 */
enum GameInput player_game_input(GameInputSourceArgs a)
{
    while (true)
    {
        enum KeyboardInput key = keyboard_input();

        switch (key)
        {
        case KEY_W: case KEY_ARROW_UP: return UP_INPUT;
        case KEY_A: case KEY_ARROW_LEFT: return LEFT_INPUT;
        case KEY_S: case KEY_ARROW_DOWN: return DOWN_INPUT;
        case KEY_D: case KEY_ARROW_RIGHT: return RIGHT_INPUT;
        case KEY_Q: case KEY_ESCAPE: case KEY_BACKSPACE: return QUIT_INPUT;
        case KEY_ENTER: case KEY_SPACE: return MOVE_INPUT;
        default: continue;
        }
    }
}

struct AIBrain;

/**
 * Tipo da função responsável
 * por ser o "cortex" da IA.
 * Responsável pela análise "visual",
 * análise "espacial" e tomada de decisão.
 */
typedef void (*AIBrainCortex)(struct AIBrain *brain);

/**
 * Macro que define o valor
 * do `goal` que indica que a IA
 * ainda não pensou em uma jogada.
 */
#define AI_THINKING_STATE vec2(-1, -1)

/**
 * Macro para detectar se a IA
 * ainda precisa pensar em uma jogada.
 *
 * Veja: `AI_THINKING_STATE`
 */
#define is_ai_thinking(v) ((v.x < 0) || (v.y < 0))

/**
 * A "cabeça" da IA.
 */
struct AIBrain
{
    /**
     * Os "olhos" da IA,
     * ela não pode modificar
     * diretamente, apenas "ver".
     */
    struct GameState *view;
    /**
     * O "tomador de decisão" da IA.
     */
    AIBrainCortex cortex;
    /**
     * Quando os valores de goal
     * forem negativos, significa
     * que a IA ainda não pensou,
     * enquanto valores positivos (0-2),
     * indicam onde ela vai jogar.
     *
     * Veja: `AI_THINKING_STATE`
     */
    struct Vec2 goal;
};

/**
 * Construtor para `AIBrain`.
 */
struct AIBrain create_ai_brain(struct GameState *state, AIBrainCortex cortex)
{
    return (struct AIBrain)
    {
        .view = state,
        .cortex = cortex,
        .goal = AI_THINKING_STATE,
    };
}

/**
 * Executa o "cortex" da IA.
 */
void ai_think(struct AIBrain *brain)
{
    brain->cortex(brain);
}

/**
 * Algoritmo para fazer a IA
 * "andar" pelo tabuleiro como
 * um jogador.
 */
enum GameInput ai_walk(struct AIBrain *brain)
{
    struct Vec2 where_i_am = brain->view->selection;
    struct Vec2 where_i_want = brain->goal;
    struct Vec2 direction =
    {
        where_i_want.x - where_i_am.x,
        where_i_want.y - where_i_am.y,
    };
    struct Vec2 distance = {abs(direction.x), abs(direction.y)};

    if (distance.x > distance.y)
    {
        if (direction.x < 0)
            return LEFT_INPUT;
        return RIGHT_INPUT;
    }
    if (direction.y < 0)
        return UP_INPUT;
    return DOWN_INPUT;
}

/**
 * Simula alguém pensando e jogando,
 * enviando inputs gerados por software.
 *
 * Compatível com o tipo `GameInputSourceExecutor`.
 */
enum GameInput ai_game_input(GameInputSourceArgs a)
{
    struct AIBrain *brain = a;
    
    bool am_i_thinking = is_ai_thinking(brain->goal);
    if (am_i_thinking)
    {
        block_delay((rand() % 300) + 300);
        ai_think(brain);
    }

    bool am_i_where_i_want = (brain->goal.x == brain->view->selection.x)
        && (brain->goal.y == brain->view->selection.y);
    if (am_i_where_i_want)
    {
        block_delay(225);
        brain->goal = AI_THINKING_STATE;
        return MOVE_INPUT;
    }

    block_delay((rand() % 50) + 100);
    return ai_walk(brain);
}

/**
 * Cortex ruim.
 *
 * Basicamente, uma burrice artificial.
 * Apenas escolhe uma célula aleatória e joga.
 */
void dumb_ai_cortex(struct AIBrain *brain)
{
    struct Vec2 randomly_choosen_cell = {0};

    do
        randomly_choosen_cell = vec2(rand() % 3, rand() % 3);
    while (game_board_cell(brain->view->board, randomly_choosen_cell) != FREE_MOVE);

    brain->goal = randomly_choosen_cell;
}

/**
 * Parte do cortex mediano/bom.
 *
 * Diz se a linha analisada é
 * potencialmente ruim de jogar.
 */
bool avarage_ai_is_line_potentially_useless(MovePrint my_moves, MovePrint enemy_moves)
{
    return (!test_move_print_purity(enemy_moves, my_moves))
        || move_print_count(enemy_moves) == 0;
}

/**
 * Parte do cortex mediano/bom.
 *
 * Testa se alguém vai ganhar com
 * alguma jogada, o teste é feito
 * pela perspectiva do `testing`
 */
bool avarage_ai_test_winner(MovePrint testing, MovePrint opponent)
{
    return move_print_count(testing) == 2
        && test_move_print_purity(testing, opponent);
}

/**
 * Parte do cortex mediano/bom.
 *
 * Enxerga quais jogadas faltam em uma linha.
 */
MovePrint avarage_ai_see_missing_moves(MovePrint my_moves, int match_print_i)
{
    return my_moves ^ match_move_prints[match_print_i];
}

/**
 * Parte do cortex mediano/bom.
 *
 * Uma memória temporária para
 * armazenar possíveis jogadas.
 */
struct AvarageAIMoveOptions
{
    size_t len;
    struct Vec2 moves[9];
    MovePrint __stored_moves;
};

/**
 * Parte do cortex mediano/bom.
 *
 * Adiciona mais uma jogada em um
 * `AvarageAIMoveOptions` se ainda
 * não houver a jogada `move`.
 */
void avarage_ai_move_options_push(struct AvarageAIMoveOptions *opts, struct Vec2 move)
{
    if (!move_print_inspec(opts->__stored_moves, move))
    {
        edit_move_print(&opts->__stored_moves, move, true);
        opts->moves[opts->len] = move;
        opts->len++;
    }
}

/**
 * Parte do cortex mediano/bom.
 *
 * Pega aleatoriamente uma
 * jogada de um `AvarageAIMoveOptions`
 */
struct Vec2 avarage_ai_move_options_pick(struct AvarageAIMoveOptions *opts)
{
    if (opts->len == 0) return vec2(-1, -1);

    if (opts->len == 1) return opts->moves[0];
    return opts->moves[rand() % opts->len];
}

/**
 * Cortex mediano/bom.
 *
 * Em resumo, o algoritmo consiste
 * em sempre tentar bloquear o oponente,
 * e ganhar assim que aparecer uma
 * oportunidade, mas não é
 * estratégico o suficiente
 * para ser impossível.
 */
void avarage_ai_cortex(struct AIBrain *brain)
{
    bool should_i_start = brain->view->moves == 0;
    if (should_i_start)
    {
        brain->goal = vec2(rand() % 3, rand() % 3);
        return;
    }

    enum Actor me = brain->view->turn;
    enum Actor enemy = opponent_actor(brain->view->turn);

    struct MovePrintTriplet move_view = get_move_print_triplet(brain->view->board);
    MovePrint all_my_moves = (me == X_ACTOR) ? move_view.x : move_view.o;
    MovePrint all_enemy_moves = (enemy == X_ACTOR) ? move_view.x : move_view.o;

    struct AvarageAIMoveOptions danger_cells = {0};
    struct AvarageAIMoveOptions avarage_moves = {0};
    struct AvarageAIMoveOptions potentially_useless = {0};

    for (int i = 0; i < 8; i++)
    {
        MovePrint my_moves = all_my_moves & match_move_prints[i];
        MovePrint enemy_moves = all_enemy_moves & match_move_prints[i];

        bool will_i_win = avarage_ai_test_winner(my_moves, enemy_moves);
        if (will_i_win)
        {
            MovePrint missing_move = avarage_ai_see_missing_moves(my_moves, i);
            move_print_coords(missing_move, &brain->goal);
            return;
        }

        if (avarage_ai_is_line_potentially_useless(my_moves, enemy_moves))
        {
            MovePrint moves = avarage_ai_see_missing_moves(my_moves, i);
            uint8_t move_count = move_print_count(moves);
            struct Vec2 move_coords[3] = {0};
            move_print_coords(moves, move_coords);
            for (int i = 0; i < move_count; i++)
                avarage_ai_move_options_push(&potentially_useless, move_coords[i]);
            continue;
        }

        bool will_enemy_win = avarage_ai_test_winner(enemy_moves, my_moves);
        if (will_enemy_win)
        {
            MovePrint missing_move = avarage_ai_see_missing_moves(enemy_moves, i);
            struct Vec2 danger_cell = {0};
            move_print_coords(missing_move, &danger_cell);
            avarage_ai_move_options_push(&danger_cells, danger_cell);
            continue;
        }

        MovePrint available_moves = avarage_ai_see_missing_moves(enemy_moves, i);
        struct Vec2 available_moves_coords[2] = {0};
        move_print_coords(available_moves, available_moves_coords);

        avarage_ai_move_options_push(&avarage_moves, available_moves_coords[0]);
        avarage_ai_move_options_push(&avarage_moves, available_moves_coords[1]);
    }

    bool am_i_in_danger = danger_cells.len > 0;
    bool do_i_have_good_moves = avarage_moves.len > 0;

    if (am_i_in_danger)
        brain->goal = avarage_ai_move_options_pick(&danger_cells);
    else if (do_i_have_good_moves)
        brain->goal = avarage_ai_move_options_pick(&avarage_moves);
    else
        brain->goal = avarage_ai_move_options_pick(&potentially_useless);
}

/**
 * Estrutura que guarda
 * o tamanho em bytes (`blen`)
 * e o tamanho visual/utf-8 (`ulen`)
 */
struct UStrLenRes
{
    size_t ulen;
    size_t blen;
};

/**
 * Versão utf-8 do `strlen`, que conta
 * code points utf-8 além de contar os bytes.
 */
struct UStrLenRes ustrlen(const char *str)
{
    struct UStrLenRes ln = {0, 0};
    for (char c = *str; c != 0; str++, c = *str)
    {
        // `(c & 0xC0) == 0x80` testa se
        // o byte segue o padrão `10xxxxxx`,
        // Se ele seguir, é só um byte de continuidade
        // do utf-8 e não precisa ser contado.
        ln.ulen += ((c & 0xC0) == 0x80) ? 0 : 1;
        ln.blen++;
    }
    return ln;
}

/**
 * Em vez de escrever texto
 * a partir do cursor, usa o
 * cursor como o centro do texto.
 */
void write_center(const char *str)
{
    struct UStrLenRes len = ustrlen(str);
    size_t visual_half = len.ulen - (len.ulen / 2);
    move_cursor(vec2(-visual_half, 0));
    fwrite(str, 1, len.blen, stdout);
}

/// Flag para negrito.
#define BOLD_FLAG ((uint8_t)(0x01))
/// Flag para esvaecido.
#define DIM_FLAG ((uint8_t)(0x02))
/// Flag para itálico.
#define ITALIC_FLAG ((uint8_t)(0x04))
/// Flag para cor do texto.
#define FOREGROUND_COLOR_FLAG ((uint8_t)0x08)
/// Flag para cor do fundo.
#define BACKGROUND_COLOR_FLAG ((uint8_t)0x10)

/**
 * Estrutura que guarda a
 * representação visual de um
 * texto para menus.
 *
 * `fmt_flags` pode conter:
 * - `BOLD_FLAG`;
 * - `DIM_FLAG`;
 * - `ITALIC_FLAG`;
 * - `FOREGROUND_COLOR_FLAG` (depende do campo `foreground_color`);
 * - `BACKGROUND_COLOR_FLAG` (depende do campo `background_color`).
 */
struct TextStyle
{
    struct Color foreground_color;
    struct Color background_color;
    uint8_t fmt_flags;
};

/**
 * Um nó de texto que
 * guarda tanto um ponteiro
 * parao o texto em si, quanto
 * seu estilo atribuído.
 */
struct TextNode
{
    struct TextStyle style;
    const char *str;
};

/**
 * Escreve um nó de texto
 * na tela, usando o cursor
 * como centro do texto, e
 * aplicando o estilo atribuído.
 */
void write_text_node(struct TextNode node)
{
    if (node.style.fmt_flags & BOLD_FLAG)
        set_bold();
    else if (node.style.fmt_flags & DIM_FLAG)
        set_dim();

    if (node.style.fmt_flags & ITALIC_FLAG)
        set_italic();

    if (node.style.fmt_flags & FOREGROUND_COLOR_FLAG)
        set_foreground_color(node.style.foreground_color);
    if (node.style.fmt_flags & BACKGROUND_COLOR_FLAG)
        set_background_color(node.style.foreground_color);

    write_center(node.str);

    if (node.style.fmt_flags != 0)
        reset_formatting();
}

/**
 * Escreve vários nós verticalmente, usando
 * o cursor como centro tanto horizontal
 * quanto vertical.
 */
void write_text_node_row(struct Vec2 offset, size_t n, struct TextNode nodes[n])
{
    offset.y -= n - (n / 2);

    set_cursor_position(offset);

    for (size_t i = 0; i < n; i++)
    {
        write_text_node(nodes[i]);
        offset.y += 1;
        set_cursor_position(offset);
    }
}

/// Nenhum estilo (estilo plano).
const struct TextStyle plain_style = {0};
/// Estilo de título.
const struct TextStyle title_style = { .fmt_flags = BOLD_FLAG, };
/// Estilo de opção.
const struct TextStyle option_style = { .fmt_flags = ITALIC_FLAG, };
/// Estilo de informação/instrução.
const struct TextStyle info_style = { .fmt_flags = DIM_FLAG | ITALIC_FLAG, };

/**
 * Possíveis opções de jogo.
 */
enum MainMenuOption
{
    QUIT_GAME,
    PLAYER_VS_PLAYER,
    PLAYER_VS_MACHINE,
    MACHINE_VS_MACHINE,
};

/**
 * Menu principal do jogo.
 * Mostra opções de jogo e
 * informa os controles.
 */
enum MainMenuOption main_menu()
{
    new_screen_frame(true);

    struct Vec2 dsize = display_size();
    struct Vec2 menu_offset = {dsize.x / 2, dsize.y / 2};

    struct TextNode menu[] = {
        {title_style, "C Tic Tac Toe"},
        {option_style, "1. Jogador vs. Jogador"},
        {option_style, "2. Jogador vs. Máquina"},
        {option_style, "3. Máquina vs. Máquina"},
        {plain_style, ""},
        {info_style, "Q Escape Backspace => Saír"},
        {plain_style, ""},
        {title_style, "Controles"},
        {info_style, "WASD ↑←↓→ => Mover"},
        {info_style, "Espaço Enter => Marcar"},
    };

    write_text_node_row(menu_offset, sizeof(menu)/sizeof(struct TextNode), menu);

    while (true)
    {
        enum KeyboardInput key = keyboard_input();

        switch (key)
        {
        case KEY_1: return PLAYER_VS_PLAYER;
        case KEY_2: return PLAYER_VS_MACHINE;
        case KEY_3: return MACHINE_VS_MACHINE;
        case KEY_ESCAPE: case KEY_Q: case KEY_BACKSPACE: return QUIT_GAME;
        default: continue;
        }
    }
}

/**
 * Um pequeno popup para
 * instruír os jogadores a
 * escolherem quem vai ser quem.
 *
 * Retorna `true` se os jogadores
 * decidirem procesguir, mas retorna
 * `false` se eles quiserem cancelar
 * a partida.
 */
bool player_vs_player_popup()
{
    new_screen_frame(true);

    struct Vec2 dsize = display_size();
    struct Vec2 menu_offset = {dsize.x / 2, dsize.y / 2};

    struct TextNode info[] = {
        {title_style, "Decidam quem vai ser quem (X ou O)"},
        {plain_style, "Quem começa (X ou O) é decidido aleatoriamente pelo jogo"},
        {plain_style, ""},
        {info_style, "Espaço Enter => Confirmar"},
        {info_style, "Q Escape Backspace => Cancelar"},
    };

    write_text_node_row(menu_offset, sizeof(info)/sizeof(struct TextNode), info);

    return blocking_confirm();
}

/**
 * Um menu que deixa o jogador
 * decidir se ele quer usar X ou O.
 *
 * `NULL_ACTOR` significa cancelamento.
 */
enum Actor player_actor_selection_menu()
{
    new_screen_frame(true);

    struct Vec2 dsize = display_size();
    struct Vec2 menu_offset = {dsize.x / 2, dsize.y / 2};

    struct TextStyle x_option =
    {
        .fmt_flags = FOREGROUND_COLOR_FLAG | ITALIC_FLAG,
        .foreground_color = actor_color(X_ACTOR),
    };

    struct TextStyle o_option =
    {
        .fmt_flags = FOREGROUND_COLOR_FLAG | ITALIC_FLAG,
        .foreground_color = actor_color(O_ACTOR),
    };

    struct TextNode menu[] = {
        {title_style, "Escolha Sua Peça"},
        {x_option, "1. Usar Peça X"},
        {o_option, "2. Usar Peça O"},
        {plain_style, ""},
        {info_style, "Escolher X ou O não garante que você vai começar"},
        {info_style, "Q Escape Backspace => Cancelar"},
    };

    write_text_node_row(menu_offset, sizeof(menu)/sizeof(struct TextNode), menu);

    while (true)
    {
        enum KeyboardInput key = keyboard_input();
        switch (key)
        {
        case KEY_1: return X_ACTOR;
        case KEY_2: return O_ACTOR;
        case KEY_Q: case KEY_ESCAPE: case KEY_BACKSPACE: return NULL_ACTOR;
        default: continue;
        }
    }
}

/**
 * Menu para o jogador escolher
 * uma IA para jogar.
 *
 * `NULL` significa cancelamento.
 */
AIBrainCortex ai_cortex_selection_menu(const char *ctitle, struct TextStyle *ctitle_style)
{
    new_screen_frame(true);

    struct Vec2 dsize = display_size();
    struct Vec2 menu_offset = {dsize.x / 2, dsize.y / 2};

    struct TextNode menu[] = {
        {
            (ctitle_style == NULL) ? title_style : *ctitle_style,
            (ctitle == NULL) ? "Escolha um Oponente" : ctitle,
        },
        {option_style, "1. Burrice Artificial (fácil)"},
        {option_style, "2. Inteligência Bloqueante (médio)"},
        {plain_style, ""},
        {info_style, "Q Escape Backspace => Cancelar"},
    };

    write_text_node_row(menu_offset, sizeof(menu)/sizeof(struct TextNode), menu);

    while (true)
    {
        enum KeyboardInput key = keyboard_input();
        switch (key)
        {
        case KEY_1: return dumb_ai_cortex;
        case KEY_2: return avarage_ai_cortex;
        case KEY_Q: case KEY_ESCAPE: case KEY_BACKSPACE: return NULL;
        default: continue;
        }
    }
}

/**
 * Mostra por mais ou menos 2 segundos um
 * popup mostrando quem vai começar
 * jogando (X ou O).
 */
void who_is_starting_popup(enum Actor starter)
{
    new_screen_frame(true);
    block_delay(200);

    struct Vec2 dsize = display_size();
    struct Vec2 menu_offset = {dsize.x / 2, dsize.y / 2};

    struct TextStyle actor_titile_style =
    {
        .fmt_flags = FOREGROUND_COLOR_FLAG | BOLD_FLAG,
        .foreground_color = actor_color(starter),
    };

    struct Vec2 title_offset = { menu_offset.x, menu_offset.y - 2 };
    set_cursor_position(title_offset);
    write_text_node((struct TextNode){actor_titile_style, "Quem Começa:"});

    struct Vec2 actor_box_offset = { menu_offset.x - 3, menu_offset.y - 1 };
    set_cursor_position(actor_box_offset);
    draw_game_cell(starter, actor_to_move(starter), true);

    block_delay(2E3 + 100);
    new_screen_frame(true);
    block_delay(200);
}

/**
 * E finalmente, a função `main` !
 */
int main()
{
    setup_terminal();

    srand(time(NULL));
    
    struct GameInputSource player =
    {
        .executor = player_game_input,
        .args = NULL,
    };

    while (true)
    {
        enum MainMenuOption opt = main_menu();

        struct GameState game =
        {
            .turn = (enum Actor)((rand() % 2) + 1),
        };

        switch (opt)
        {
        case QUIT_GAME:
            return 0;
        case PLAYER_VS_PLAYER:
        {
            if (player_vs_player_popup() == false) break;
            who_is_starting_popup(game.turn);
            while (game_event_loop(&game, player));
            break;
        }
        case PLAYER_VS_MACHINE:
        {
            CHOOSING_ACTOR_START:;
            enum Actor player_actor = player_actor_selection_menu();
            if (player_actor == NULL_ACTOR) break;
            AIBrainCortex ai_cortex = ai_cortex_selection_menu(NULL, NULL);
            if (ai_cortex == NULL) goto CHOOSING_ACTOR_START;

            struct AIBrain ai_brain = create_ai_brain(&game, ai_cortex);
            struct GameInputSource ai =
            {
                .executor = ai_game_input,
                .args = &ai_brain,
            };

            struct GameInputSource x_input = {0};
            struct GameInputSource o_input = {0};

            if (player_actor == X_ACTOR)
            {
                x_input = player;
                o_input = ai;
            }
            else
            {
                x_input = ai;
                o_input = player;
            }

            who_is_starting_popup(game.turn);
            while (game_event_loop(&game, (game.turn == X_ACTOR) ? x_input : o_input));
            break;
        }
        case MACHINE_VS_MACHINE:
        {
            struct TextStyle x_style =
            {
                .fmt_flags = FOREGROUND_COLOR_FLAG | BOLD_FLAG,
                .foreground_color = actor_color(X_ACTOR),
            };

            struct TextStyle o_style =
            {
                .fmt_flags = FOREGROUND_COLOR_FLAG | BOLD_FLAG,
                .foreground_color = actor_color(O_ACTOR),
            };

            CHOOSING_AI_START:;
            AIBrainCortex x_cortex = ai_cortex_selection_menu("Escolha Alguém Para X", &x_style);
            if (x_cortex == NULL) break;
            AIBrainCortex o_cortex = ai_cortex_selection_menu("Escolha Alguém Para O", &o_style);
            if (o_cortex == NULL) goto CHOOSING_AI_START;

            struct AIBrain x_brain = create_ai_brain(&game, x_cortex);
            struct GameInputSource x_ai =
            {
                .executor = ai_game_input,
                .args = &x_brain,
            };

            struct AIBrain o_brain = create_ai_brain(&game, o_cortex);
            struct GameInputSource o_ai =
            {
                .executor = ai_game_input,
                .args = &o_brain,
            };

            who_is_starting_popup(game.turn);
            while (game_event_loop(&game, (game.turn == X_ACTOR) ? x_ai : o_ai));
            break;
        }
        }
    }
}
