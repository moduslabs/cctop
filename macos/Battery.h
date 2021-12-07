//
// Created by Michael Schwartz on 12/6/21.
//

#ifndef CCTOP_BATTERY_H
#define CCTOP_BATTERY_H


class Battery {
public:
    void update();
    void print(bool test);
};

extern Battery battery;

#endif //CCTOP_BATTERY_H
