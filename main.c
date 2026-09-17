#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DATE_LEN 10
#define DOSE_LEN 2
// TODO: don't limit dose to 2 chars -- right now it's not working otherwise

void get_date(char* buf) {
    time_t t = time(nullptr);
    struct tm tm = *localtime(&t);

    snprintf(buf, DATE_LEN + 1, "%02d.%02d.%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("usage: took <number>\n");
        return 1;
    }

    const char* const command = argv[1];

    if (strcmp(command, "took") != 0) {
        printf("unknown command '%s'\n", command);
        return 1;
    }

    const char* const dose_str = argv[2];

    char* end;
    errno = 0;
    const auto dose = strtol(dose_str, &end, 10);

    if (errno || end == dose_str || *end != '\0') {
        printf("can't parse dose '%s'\n", dose_str);
        return 1;
    }

    printf("adding %ld mg to today's dose\n", dose);

    char date_str[DATE_LEN + 1];
    get_date(date_str);

    FILE* fp = fopen("log.csv", "a+");

    if (!fp) {
        printf("can't open log file\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);

    if (ftell(fp) == 0) {
        fprintf(fp, "%s,%ld", date_str, dose);
    } else {
        // file is not empty,
        // need to read the last line and check if it's today.
        // if it is, then add dose,
        // else just append the date and dose to the file

        const auto line_len = DATE_LEN + DOSE_LEN + 1; // date + , + dose
        char last_line[line_len + 1];

        fseek(fp, -line_len, SEEK_END);
        const auto before_last_line_pos = ftell(fp);
        auto len = fread(last_line, sizeof last_line[0], line_len, fp);
        last_line[len] = '\0';

        if (ferror(fp)) {
            printf("can't read log file\n");
            fclose(fp);
            return 1;
        }

        const char* const last_date_str = strtok(last_line, ",");
        const char* const last_dose_str = strtok(nullptr, ",");

        if (strcmp(last_date_str, date_str) == 0) {
            char* end;
            errno = 0;
            const auto last_dose = strtol(last_dose_str, &end, 10);

            if (errno != 0 || end == last_dose_str || *end != '\0') {
                printf("can't parse dose from the last log line '%s'\n", last_dose_str);
                return 1;
            }

            const auto new_last_dose = dose + last_dose;

            ftruncate(fileno(fp), before_last_line_pos + DATE_LEN + 1);
            fprintf(fp, "%ld", new_last_dose);
        } else {
            fprintf(fp, "\n%s,%ld", date_str, dose);
        }
    }

    fclose(fp);

    printf("succesfully added %ld to today's dose\n", dose);
    return 0;
}