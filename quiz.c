#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_QUEST_LEN 300
#define MAX_OPT_LEN 100

typedef struct
{
    char text[MAX_QUEST_LEN];
    char options[4][MAX_OPT_LEN];
    char correct_option;
    int timeout;
    int prize_money;
} Question;

void reset_terminal_attributes();
struct termios old_props;

void set_terminal_attributes(){
    //Change terminal properties
    tcgetattr(STDIN_FILENO, &old_props);
    atexit(reset_terminal_attributes);
    struct termios new_props = old_props;
    new_props.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_props);
}

void reset_terminal_attributes(){
    tcsetattr(STDIN_FILENO, TCSANOW, &old_props);
}

void print_formatted_question(Question question){
    printf("\n\n%s", question.text);
    for(int i = 0; i < 4; i++){
        if(question.options[i][0] != '\0'){
            printf("%c. %s",('A' + i), question.options[i]);
        }
    }
    printf("\nTime limit: %d seconds.", question.timeout);
    printf("\nEnter your answer (A, B, C, or D) or L for Lifeline: ");
    fflush(stdout);
}

int read_char_until(const struct timespec *deadline, char *input)
{
    while (1) {
        struct timespec now;

        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            return -1;
        }

        time_t seconds = deadline->tv_sec - now.tv_sec;
        long nanoseconds = deadline->tv_nsec - now.tv_nsec;

        if (nanoseconds < 0) {
            seconds--;
            nanoseconds += 1000000000L;
        }

        if (seconds < 0 || (seconds == 0 && nanoseconds <= 0)) {
            return 0;
        }

        struct timeval remaining = {
            .tv_sec = seconds,
            .tv_usec = nanoseconds / 1000
        };

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &remaining);

        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (ready == 0) {
            return 0;
        }

        char ch;
        ssize_t bytes_read = read(STDIN_FILENO, &ch, 1);

        if (bytes_read == 1) {
            if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
        return -1;
        }

        if (now.tv_sec > deadline->tv_sec ||
        (now.tv_sec == deadline->tv_sec &&
        now.tv_nsec >= deadline->tv_nsec)) {
        return 0;
        }

        if (ch == '\n' || ch == '\r') {
        continue;
        }

        *input = ch;
        return 1;
        }

        if (bytes_read == -1 && errno == EINTR) {
            continue;
        }

        return -1;
    }
}

int use_LifeLine(Question* question, int* Lifeline,
                 const struct timespec *deadline)
{
    printf("\n\nAvailable Lifelines:");
    if (Lifeline[0]) printf("\n1. Fifty-Fifty (50/50)");
    if (Lifeline[1]) printf("\n2. Skip the Question");
    printf("\nChoose a lifeline or 0 to return: ");
    fflush(stdout);

    char ch;
    int input_status = read_char_until(deadline, &ch);

    if (input_status == 0) return -1;
    if (input_status == -1) return -2;

    printf("%c", ch);

    switch(ch)
    {
        case '1':
            if(Lifeline[0]){
                Lifeline[0] = 0;
                int removed = 0;

                while(removed < 2){
                    int num = rand() % 4;

                    if ((num + 'A') != question->correct_option &&
                       question->options[num][0] != '\0') {
                        question->options[num][0] = '\0';
                        removed++;
                    }
                }
                return 1;
            }
            break;

        case '2':
            if(Lifeline[1]){
                Lifeline[1] = 0;
                return 2;
            }
            break;

        default:
            printf("\nReturning to the question");
            break;
    }

    return 0;
}

void play_game(Question* question, int no_of_questions)
{
    int money_won = 0;
    int Lifeline[] = {1, 1};
    int game_over = 0;

    for(int i = 0; i < no_of_questions; i++){

        struct timespec deadline;

        if(clock_gettime(CLOCK_MONOTONIC, &deadline) == -1){
            perror("Unable to start question timer");
            break;
        }

        deadline.tv_sec += question[i].timeout;

        print_formatted_question(question[i]);

        while(1){

            char ch;
            int input_status = read_char_until(&deadline, &ch);

            if(input_status == 0){
                printf("\n\nTime's up! You did not answer in time.");
                game_over = 1;
                break;
            }

            if(input_status == -1){
                perror("\nInput error");
                game_over = 1;
                break;
            }

            printf("%c", ch);
            ch = (char)toupper((unsigned char)ch);

            if(ch == 'L'){

                int value = use_LifeLine(&question[i], Lifeline, &deadline);

                if(value == -1){
                    printf("\n\nTime's up! You did not answer in time.");
                    game_over = 1;
                    break;
                }

                if(value == -2){
                    perror("\nInput error");
                    game_over = 1;
                    break;
                }

                if(value == 2){
                    printf("\nQuestion skipped.");
                    break;
                }

                print_formatted_question(question[i]);
                continue;
            }

            if(ch == question[i].correct_option){
                money_won = question[i].prize_money;
                printf("\n\nCorrect. Congratulations! You have won: Rs %d",
                       money_won);
            }
            else{
                printf("\n\nWrong! The Correct answer is %c",
                       question[i].correct_option);
                game_over = 1;
            }

            break;
        }

        if(game_over){
            break;
        }
    }

    printf("\n\nGame over. Your total earnings are: %d\n", money_won);
}

int read_questions(char* file_name, Question** questions){
    FILE *file = fopen(file_name, "r");
    if(file == NULL){
        printf("\nUnable to open the questions file");
        exit(0);
    }

    char str[MAX_QUEST_LEN];
    int no_of_lines = 0;
    while(fgets(str, MAX_QUEST_LEN, file)){
        no_of_lines++;
    }
    int no_of_questions = no_of_lines / 8;
    *questions = (Question *) malloc(no_of_questions * sizeof(Question));

    rewind(file);

    for(int i = 0;i < no_of_questions; i++){
        fgets((*questions)[i].text, MAX_QUEST_LEN, file);
        for(int j = 0;j < 4; j++){
            fgets((*questions)[i].options[j], MAX_OPT_LEN, file);
        }
        char option[10];
        fgets(option, 10, file);
        (*questions)[i].correct_option = option[0];

        char timeout[10];
        fgets(timeout, 10, file);
        (*questions)[i].timeout = atoi(timeout);

        char prize_money[10];
        fgets(prize_money, 10, file);
        (*questions)[i].prize_money = atoi(prize_money);
    }

    fclose(file);
    return no_of_questions;
}

int main()
{
    srand(time(NULL));
    set_terminal_attributes();
    Question* questions;
    int no_of_questions = read_questions("questions.txt", &questions);

    play_game(questions, no_of_questions);
    exit(0);

    return 0;
}