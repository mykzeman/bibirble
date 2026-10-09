#pragma once

// Whole years between a birth date and today (both as year, month 1-12,
// day 1-31), or -1 if the birth date is invalid or in the future. Used by
// the R18 mode date-of-birth check; kept free of wxWidgets so it's easy to
// test.
inline int AgeOnDate(int birthYear, int birthMonth, int birthDay,
                     int todayYear, int todayMonth, int todayDay) {
    static const int kDaysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (birthMonth < 1 || birthMonth > 12 || birthDay < 1 ||
        birthDay > kDaysInMonth[birthMonth - 1]) {
        return -1;
    }
    bool leap = (birthYear % 4 == 0 && birthYear % 100 != 0) || birthYear % 400 == 0;
    if (birthMonth == 2 && birthDay == 29 && !leap) return -1;

    int age = todayYear - birthYear;
    if (todayMonth < birthMonth || (todayMonth == birthMonth && todayDay < birthDay)) {
        --age;
    }
    return age < 0 ? -1 : age;
}

inline constexpr int kR18MinimumAge = 18;
