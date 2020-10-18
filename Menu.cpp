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

// Initialize static variable
bool Engine::progMemEntries = true;

static inline void *readPtr(const Item_t *item, uint8_t offset) {
    uint8_t *targetAddress = ((uint8_t *)item) + offset;
    return Engine::progMemEntries ? pgm_read_ptr(targetAddress) : *((void **)targetAddress);
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

static inline void updateItemInfo(Engine &en) {
    en.currentItemInfo.siblings = 0;

    const Item_t *i = Engine::getChild(Engine::getParent(en.currentItemInfo.item));
    for (; i != NULL; i = Engine::getNext(i)) {
        ++en.currentItemInfo.siblings;
        if (i == en.currentItemInfo.item) {
            en.currentItemInfo.position = en.currentItemInfo.siblings;
        }
    }
}

// ----------------------------------------------------------------------------

void Engine::navigate(const Item_t *targetItem) {
    if (targetItem != NULL) {
        // navigating back to parent
        if (targetItem == getParent(currentItemInfo.item)) {
            // exit/save callback decide if we should go back
            if (!executeCallbackAction(actionParent)) {
                return;
            }
            forceNewRender = true;
        }

        // Save navigation change
        previousItem = currentItemInfo.item;
        currentItemInfo.item = targetItem;
        //executeCallbackAction(actionLabel);
    }
}

// ----------------------------------------------------------------------------

void Engine::invoke() {
    const Item_t *child = getChild(currentItemInfo.item);
    if (child != NULL) { // navigate to registered submenuitem
        navigate(child);
        forceNewRender = true;
    } else {
        // call trigger in selected item that has no child
        executeCallbackAction(actionTrigger);
    }
}

// ----------------------------------------------------------------------------

bool Engine::executeCallbackAction(const Action_t action, const Item_t *menuItem) {
    if (menuItem != NULL) {
        const Callback_t *callback = (Callback_t *)readPtr(menuItem, offsetof(Item_t, Callback));

        if (callback != NULL) {
            return callback(action, *this);
        }
    }
    return true;
}

void Engine::render(const RenderCallback_t *render, uint8_t maxDisplayedMenuItems) {
    if (currentItemInfo.item == NULL) {
        return;
    }

    const uint8_t center = maxDisplayedMenuItems >> 1;
    updateItemInfo(*this);

    uint8_t start = 0;
    if (currentItemInfo.position >= (uint8_t)(currentItemInfo.siblings - center)) { // at end
        start = currentItemInfo.siblings - maxDisplayedMenuItems;
    } else {
        start = currentItemInfo.position - center;
        // center if odd
        start -= maxDisplayedMenuItems & 0x01;
    }

    if (start & 0x80)
        start = 0; // prevent underflow

    if (forceNewRender) {
        // We need to clear the screen first
        // To make some optimization possible we will also give the count of entries to render afterwards
        // This might be used to not clear the complete screen
        render(*this, NULL);
    } else if (prevStart != start) {
        // If the start offset changed then we have to render once everything!
        forceNewRender = true;
    }

    prevStart = start;

    const Item_t *currentItemBackup = currentItemInfo.item;
    // Calculate the relative index of the shown elements (starting at 1)
    const uint8_t currentPositionBackup = currentItemInfo.position - start;

    // Can we optimize the element render count?
    if (!forceNewRender) {
        // Convert index to starting at 0 index
        uint8_t renderPosLow = currentPositionBackup - 1;

        // Going forward? If so we have to move the render frame one position up
        if (previousItem && previousItem == getPrev(currentItemBackup)) {
            --renderPosLow;
        }

        // Adjust start index of rendering
        start += renderPosLow;
        // We need to apply this offset here too
        currentItemInfo.position = renderPosLow;
        // finally set render bound up to which element rendering is needed
        renderPosLow += 2;
        if (renderPosLow < maxDisplayedMenuItems) {
            maxDisplayedMenuItems = renderPosLow;
        }
    } else {
        // We need to render everything, set start position and end condition accordingly
        currentItemInfo.position = 0;
    }

    // first item in current menu level
    currentItemInfo.item = getChild(getParent(currentItemInfo.item));

    // Iterate to the start item
    for (; start > 0; --start) {
        // get the next item
        currentItemInfo.item = getNext(currentItemInfo.item);
    }

    // We know that we need to render at least one item -> lets use do while
    do {
        // Update the local index (starting at 1)
        ++currentItemInfo.position;
        render(*this, currentItemBackup);
        executeCallbackAction(actionDisplay);

        // get the next item
        currentItemInfo.item = getNext(currentItemInfo.item);
    } while (currentItemInfo.item != NULL && currentItemInfo.position < maxDisplayedMenuItems);

    // Restore our currentItem information
    currentItemInfo.item = currentItemBackup;
    currentItemInfo.position = currentPositionBackup;

    forceNewRender = false;
}

// ----------------------------------------------------------------------------

}; // namespace Menu

// ----------------------------------------------------------------------------
