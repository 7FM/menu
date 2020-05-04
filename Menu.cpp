// ----------------------------------------------------------------------------
// MicroMenu
// Copyright (c) 2014 karl@pitrich.com
// All rights reserved.
// License: MIT
// ----------------------------------------------------------------------------

#include "Menu.h"
#include <stddef.h>

// ----------------------------------------------------------------------------

namespace Menu {
// ----------------------------------------------------------------------------

static inline void *readPtr(const Item_t *item, uint8_t offset) {
    return pgm_read_ptr(((uint8_t *)item) + offset);
}

const char *Engine::getLabel(const Item_t *item) {
    return (const char *)readPtr(item, offsetof(Item_t, Label));
}

const Item_t *Engine::getPrev(const Item_t *item) {
    return (const Item_t *)readPtr(item, offsetof(Item_t, Previous));
}

const Item_t *Engine::getNext(const Item_t *item) {
    return (const Item_t *)readPtr(item, offsetof(Item_t, Next));
}

const Item_t *Engine::getParent(const Item_t *item) {
    return (const Item_t *)readPtr(item, offsetof(Item_t, Parent));
}

const Item_t *Engine::getChild(const Item_t *item) {
    return (const Item_t *)readPtr(item, offsetof(Item_t, Child));
}

Info_t Engine::getItemInfo(const Item_t *item) {
    Info_t result = {0, 0};
    const Item_t *i = getChild(getParent(item));
    for (; i != NULL; i = getNext(i)) {
        ++result.siblings;
        if (i == item) {
            result.position = result.siblings;
        }
    }

    return result;
}

// ----------------------------------------------------------------------------

void Engine::navigate(const Item_t *targetItem) {
    if (targetItem != NULL) {
        // navigating back to parent
        if (targetItem == getParent(currentItem)) {
            lastInvokedItem = NULL;
            // exit/save callback decide if we should go back
            if (!executeCallbackAction(actionParent)) {
                return;
            }
            forceNewRender = true;
        }

        // Save navigation change
        previousItem = currentItem;
        currentItem = targetItem;
        executeCallbackAction(actionLabel);
    }
}

// ----------------------------------------------------------------------------

void Engine::invoke() {
    bool preventTrigger = false;

    /*
    if (lastInvokedItem != currentItem) { // prevent 'invoke' twice in a row
        lastInvokedItem = currentItem;
        preventTrigger = true; // don't invoke 'trigger' at first Display event
        executeCallbackAction(actionDisplay);
    }
    */

    const Item_t *child = getChild(currentItem);
    if (child != NULL) { // navigate to registered submenuitem
        navigate(child);
        forceNewRender = true;
    } else { // call trigger in already selected item that has no child
        if (!preventTrigger) {
            executeCallbackAction(actionTrigger);
        }
    }
}

// ----------------------------------------------------------------------------

bool Engine::executeCallbackAction(const Action_t action) {
    if (currentItem != NULL) {
        Callback_t callback = (Callback_t)readPtr(currentItem, offsetof(Item_t, Callback));

        if (callback != NULL) {
            return (*callback)(action);
        }
    }
    return true;
}

void Engine::render(const RenderCallback_t render, uint8_t maxDisplayedMenuItems) {
    if (currentItem == NULL) {
        return;
    }

    const uint8_t center = maxDisplayedMenuItems >> 1;
    Info_t mi = getItemInfo(currentItem);

    uint8_t start = 0;
    if (mi.position >= (mi.siblings - center)) { // at end
        start = mi.siblings - maxDisplayedMenuItems;
    } else {
        start = mi.position - center;
        if (maxDisplayedMenuItems & 0x01)
            start--; // center if odd
    }

    if (start & 0x80)
        start = 0; // prevent overflow

    if (forceNewRender) {
        // We need to clear the screen first
        // To make some optimization possible we will also give the count of entries to render afterwards
        // This might be used to not clear the complete screen
        render(NULL, mi.siblings);
    } else if (start) {
        forceNewRender = true;
    }

    uint8_t renderPosLow = mi.position - 1;

    // Going forward? If so we have to move the render frame one position up
    if (!forceNewRender && previousItem && previousItem == getPrev(currentItem)) {
        --renderPosLow;
    }

    uint8_t itemCount = 0;
    // first item in current menu level
    for (const Item_t *i = getChild(getParent(currentItem)); i != NULL && itemCount < maxDisplayedMenuItems + start; i = getNext(i)) {
        if (itemCount >= start && (forceNewRender || (itemCount >= renderPosLow && itemCount <= renderPosLow + 1)))
            render(i, itemCount - start);
        ++itemCount;
    }

    forceNewRender = false;
}

// ----------------------------------------------------------------------------

}; // namespace Menu

// ----------------------------------------------------------------------------
