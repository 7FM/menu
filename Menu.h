// ----------------------------------------------------------------------------
// MicroMenu
// Copyright (c) 2014 karl@pitrich.com
// All rights reserved.
// License: MIT
// ----------------------------------------------------------------------------

#ifndef __have_menu_h__
#define __have_menu_h__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdlib.h>
#endif

namespace Menu {
typedef enum : uint8_t {
    actionTrigger = (1 << 0), // trigger was pressed while menu was already active
    actionParent = (1 << 1),  // before moving to parent, useful for e.g. "save y/n?" or autosave
    actionDisplay = (1 << 2), // display the value of a menu item if exists
    actionCustom = (1 << 7)
} Action_t;

class Engine;

typedef bool Callback_t(Action_t, Engine &);

typedef struct Item_s {
    const struct Item_s *Next;
    const struct Item_s *Previous;
    const struct Item_s *Parent;
    const struct Item_s *Child;
    const Callback_t *Callback;
    const char *Label;
} __attribute__((packed)) Item_t;

typedef struct {
    uint8_t siblings;
    uint8_t position;
    const Item_t *item;
} __attribute__((packed)) Info_t;

typedef void RenderCallback_t(const Engine &engine, const Item_t *selectedItem);

class Engine {
  public:
    Info_t currentItemInfo = {0, 0, NULL};
    const Item_t *previousItem = NULL;
    uint8_t prevStart = 0;

    bool forceNewRender = true;

  public:
    Engine(const Item_t *startItem) {
        currentItemInfo.item = startItem;
    }
    void navigate(const Item_t *targetItem);
    void invoke();
    bool executeCallbackAction(const Action_t action, const Item_t *menuItem);
    inline bool executeCallbackAction(const Action_t action) {
        return executeCallbackAction(action, currentItemInfo.item);
    }
    void render(const RenderCallback_t *render, uint8_t maxDisplayedMenuItems);

  public:
    // static variable to denote if Item_t entries are stored in progmem
    static bool progMemEntries;

    static const char *getLabel(const Item_t *item);
    static const Item_t *getPrev(const Item_t *item);
    static const Item_t *getNext(const Item_t *item);
    // We expect getParent to never be NULL!
    static const Item_t *getParent(const Item_t *item);
    static const Item_t *getChild(const Item_t *item);
};

}; // end namespace Menu

// ----------------------------------------------------------------------------

#define var(z) _str##z##var
#define decl(x) var(x)
#define dummyVariable() decl(__LINE__)

#define MenuItem(Name, Label, Next, Previous, Parent, Child, Callback) \
    extern const Menu::Item_t Next, Previous, Parent, Child;           \
    static const char dummyVariable()[] PROGMEM = Label;               \
    const Menu::Item_t Name PROGMEM = {                                \
        &Next, &Previous, &Parent, &Child,                             \
        &Callback,                                                     \
        dummyVariable()}

#define MenuItemPtr(Name, Label, Next, Previous, Parent, Child, Callback) \
    static const char dummyVariable()[] PROGMEM = Label;                  \
    const Menu::Item_t Name PROGMEM = {                                   \
        Next, Previous, Parent, Child,                                    \
        Callback,                                                         \
        dummyVariable()}

// ----------------------------------------------------------------------------

#endif // __have_menu_h__

// ----------------------------------------------------------------------------

/*!
  template for callback handler

void menuCallback(menuAction_t action) {  
  if (action == Menu::actionDisplay) {
    // initialy entering this menu item
  }

  if (action == Menu::actionTrigger) {
    // click on already active item
  }

  if (action == Menu::actionLabel) {
    // show thy label but don't do anything yet
  }

  if (action == Menu::actionParent) { 
    // navigating to self->parent
  }
}
*/
