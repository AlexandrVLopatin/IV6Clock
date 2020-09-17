#ifndef MENU_H
#define MENU_H

#include <DS3231.h>

enum
{
    ENCODER_ROTATION_RIGHT,
    ENCODER_ROTATION_LEFT,
};

class ActivityManager;

class Activity
{
  public:
    virtual void init() = 0;
    virtual void render() = 0;
    virtual void rotate(uint8_t direction) = 0;
    virtual void press() = 0;

    explicit Activity(ActivityManager *activity_manager)
    {
        this->_activity_manager = activity_manager;
        this->_back_activity = nullptr;
    }

    void set_back_activity(Activity *back_activity)
    {
        this->_back_activity = back_activity;
    }

  protected:
    ActivityManager *_activity_manager;
    Activity *_back_activity;
};

class ActivityManager
{
  public:
    Activity *current_activity;

    void set_current(Activity *activity)
    {
        activity->init();
        this->current_activity = activity;
    }

    void render() const
    {
        this->current_activity->render();
    }
};

typedef struct _menu_item
{
    uint8_t title;
    Activity *activity;

    void (*callback)();

} menu_item_t;

typedef struct _menu
{
    uint8_t size;
    menu_item_t items[10];
} menu_t;

class MenuActivity : public Activity
{
  public:
    using Activity::Activity;

    void init() override = 0;
    void render() override = 0;
    void rotate(uint8_t direction) override
    {
        if (direction == ENCODER_ROTATION_LEFT)
        {
            if (this->_index > 0)
                this->_index--;
            else
                this->_index = this->menu->size - 1;
        }
        else if (direction == ENCODER_ROTATION_RIGHT)
        {
            if (this->_index < this->menu->size - 1)
                this->_index++;
            else
                this->_index = 0;
        }
    }

    void press() override
    {
        if (this->menu->items[this->_index].activity != nullptr)
        {
            this->_activity_manager->set_current(this->menu->items[this->_index].activity);
        }
        else if (this->menu->items[this->_index].callback != nullptr)
        {
            this->menu->items[this->_index].callback();
        }
    }

    void set_index(uint8_t index)
    {
        this->_index = index;
    }

    uint8_t get_index() const
    {
        return this->_index;
    }

    void set_menu(const menu_t *_menu)
    {
        this->menu = _menu;
    }

    const menu_t *menu;
    uint8_t _index = 0;
};

class ClockActivity : public Activity
{
  public:
    using Activity::Activity;

    void init() override{};
    void render() override;
    void rotate(uint8_t direction) override;
    void press() override;

  private:
    unsigned long int mode_render_timer;
    uint8_t mode = 0;
};

class MainMenuActivity : public MenuActivity
{
  public:
    using MenuActivity::MenuActivity;

    void init() override{};

    void render() override;
};

enum
{
    TIME_SETUP_MODE_HOUR,
    TIME_SETUP_MODE_MINUTE,
};

class TimeSetupActivity : public Activity
{
  public:
    using Activity::Activity;

    void init() override
    {
        this->mode = TIME_SETUP_MODE_HOUR;

        bool clock_h12 = false;
        bool clock_PM = false;

        this->hour = this->_clock->getHour(clock_h12, clock_PM);
        this->minute = this->_clock->getMinute();
    }

    void render() override;

    void set_clock(DS3231 *clk)
    {
        this->_clock = clk;
    }

    void rotate(uint8_t direction) override
    {
        if (this->mode == TIME_SETUP_MODE_HOUR)
        {
            if (direction == ENCODER_ROTATION_RIGHT)
            {
                if (this->hour < 23)
                    this->hour = this->hour + 1;
                else
                    this->hour = 0;
            }
            else
            {
                if (this->hour > 0)
                    this->hour = this->hour - 1;
                else
                    this->hour = 23;
            }
        }
        else if (this->mode == TIME_SETUP_MODE_MINUTE)
        {
            if (direction == ENCODER_ROTATION_RIGHT)
            {
                if (this->minute < 59)
                    this->minute++;
                else
                    this->minute = 0;
            }
            else
            {
                if (this->minute > 0)
                    this->minute--;
                else
                    this->minute = 59;
            }
        }
    }

    void press() override
    {
        if (this->mode == TIME_SETUP_MODE_HOUR)
        {
            this->mode = TIME_SETUP_MODE_MINUTE;
        }
        else if (this->mode == TIME_SETUP_MODE_MINUTE)
        {
            this->write_time();

            if (this->_back_activity != nullptr)
            {
                this->_activity_manager->set_current(_back_activity);
            }
        }
        else
        {
            this->mode = TIME_SETUP_MODE_HOUR;
        }
    }

    void write_time()
    {
        this->_clock->setHour(this->hour);
        this->_clock->setMinute(this->minute);
        this->_clock->setSecond(0);
    }

  private:
    DS3231 *_clock;

    uint8_t mode;
    uint8_t hour;
    uint8_t minute;
};

enum
{
    COLOR_SETUP_MODE_COLOR,
    COLOR_SETUP_MODE_BRIGHTNESS,
};

class ColorSetupActivity : public Activity
{
  public:
    using Activity::Activity;

    void init() override;
    void render() override;

    void rotate(uint8_t direction) override
    {
        if (this->mode == COLOR_SETUP_MODE_COLOR)
        {
            if (direction == ENCODER_ROTATION_RIGHT)
            {
                if (this->color < 6)
                    this->color = this->color + 1;
                else
                    this->color = 0;
            }
            else
            {
                if (this->color > 0)
                    this->color = this->color - 1;
                else
                    this->color = 6;
            }
        }
        else if (this->mode == COLOR_SETUP_MODE_BRIGHTNESS)
        {
            if (direction == ENCODER_ROTATION_RIGHT)
            {
                if (this->brighness < 9)
                    this->brighness++;
                else
                    this->brighness = 0;
            }
            else
            {
                if (this->brighness > 0)
                    this->brighness--;
                else
                    this->brighness = 9;
            }
        }
    }

    void press() override
    {
        if (this->mode == COLOR_SETUP_MODE_COLOR)
        {
            this->mode = COLOR_SETUP_MODE_BRIGHTNESS;
        }
        else if (this->mode == COLOR_SETUP_MODE_BRIGHTNESS)
        {
            this->write_color();

            if (this->_back_activity != nullptr)
            {
                this->_activity_manager->set_current(_back_activity);
            }
        }
        else
        {
            this->mode = COLOR_SETUP_MODE_COLOR;
        }
    }

    void write_color() const;

  private:
    uint8_t mode;

    uint8_t color;
    uint8_t brighness;
};

#endif