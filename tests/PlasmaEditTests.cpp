#include "PlasmaEdit.h"
#include "PlasmaScene.h"
#include <cstdio>
#include <vector>

namespace {
struct Caption {
    using Validator = std::function<bool(std::u16string_view)>;
    std::u16string text;
    std::vector<std::string> events;
    std::vector<std::u16string> validated;
    Validator validator;
    bool refreshValue = true;
    plasma::Edit edit;
    explicit Caption(Validator validator = {}) : validator(std::move(validator)), edit(ports()) {}
    plasma::Edit::Caption ports() {
        plasma::Edit::Caption result{
            [this] { return text; },
            [this](const std::u16string &value) { text = value; },
            [this] { events.emplace_back("layout"); },
            [this] { events.emplace_back("change"); },
            [this] { events.emplace_back("focus-clear"); },
            [this](bool value) { refreshValue &= value; events.emplace_back("refresh"); }};
        if (validator)
            result.validate = [this](std::u16string_view value) { validated.emplace_back(value); return validator(value); };
        return result;
    }
};
}

int plasmaEditTests() {
    int failures = 0;
    const auto check = [&](bool pass,const char *message) {
        if (!pass) { ++failures; std::printf("FAIL UTF16 Edit: %s\n",message); }
    };
    {
        Caption c;
        c.edit.character('A'); c.edit.character('B');
        check(c.text == u"AB" && c.edit.caret == 2 && c.edit.selection == 0 &&
                  c.events == std::vector<std::string>{"layout","change","layout","change"},
              "separately delivered character units synchronously publish and notify once each");
        c.events.clear(); c.edit.dirty = false;
        c.edit.character(0x001f);
        check(c.text == u"AB" && !c.edit.dirty && c.events.empty(),"below-space unit returns before dirty or mutation");
        c.edit.character(0x007f); c.edit.character(0xd800); c.edit.character(0x0100);
        check(c.text == std::u16string({u'A',u'B',char16_t(0x7f),char16_t(0xd800),char16_t(0x100)}) && c.edit.caret == 5,
              "DEL, surrogate and low-byte-zero units remain literal UTF16 units");
        for (int i = 0; i < 12; ++i) c.edit.character('X');
        check(c.text.size() == 17 && c.edit.caret == 17,"editor does not impose the separate Create visibility length bound");
    }
    {
        Caption c; c.text = u"AB"; c.edit.caret = 2; c.edit.selection = -2;
        c.edit.character('C');
        check(c.text == u"C" && c.edit.caret == 1 && c.edit.selection == 0 && c.refreshValue &&
                  c.events == std::vector<std::string>{"refresh","change","layout","layout","change"},
              "selection replacement notifies deletion before insertion, with their separate layout ordering");
    }
    {
        Caption c([](auto) { return false; }); c.text = u"AB"; c.edit.caret = 2; c.edit.selection = -2;
        c.edit.character('X');
        check(c.text == u"AB" && c.edit.caret == 2 && c.edit.selection == -2 && c.events.empty() &&
                  c.validated == std::vector<std::u16string>{u"X"},
              "incoming-unit rejection happens before selected text is erased");
    }
    {
        Caption c([](auto value) { return value.size() == 1; });
        c.text = u"ABCD"; c.edit.caret = 1; c.edit.selection = 2;
        c.edit.character('X');
        check(c.text == u"AD" && c.edit.caret == 1 && c.edit.selection == 0 &&
                  c.validated == std::vector<std::u16string>{u"X",u"AXD"} &&
                  c.events == std::vector<std::string>{"refresh","change","layout"},
              "candidate rejection preserves the already completed selection deletion");
    }
    {
        Caption c([](auto) { return true; }); c.text = u"AB"; c.edit.caret = 2;
        c.edit.character('C');
        check(c.validated == std::vector<std::u16string>{u"ABC"},"unselected insertion validates only the full candidate");
    }
    {
        Caption c; c.text = u"AB"; c.edit.caret = 2;
        c.edit.keyDown(0x2e,false);
        check(c.text == u"AB" && c.events == std::vector<std::string>{"refresh","change","layout","layout"},
              "Delete at end still refreshes, notifies and performs both reached layouts");
        c.events.clear();
        c.edit.keyDown(0x08,false);
        check(c.text == u"A" && c.edit.caret == 1 && c.events == std::vector<std::string>{"refresh","layout","change"},
              "unselected Backspace erases one unit and lays out before notifying");
        c.events.clear(); c.edit.selection = -1;
        c.edit.keyDown(0x08,false);
        check(c.text.empty() && c.edit.caret == 0 && c.edit.selection == 0 &&
                  c.events == std::vector<std::string>{"refresh","change","layout","layout"},
              "selected Backspace follows the deletion helper and its extra key layout");
        c.events.clear(); c.edit.dirty = false;
        c.edit.keyDown(0x08,false);
        check(c.events.empty() && c.edit.dirty,"left-edge Backspace stops after clamp and dirty");
    }
    {
        Caption c; c.text = u"ABCD"; c.edit.caret = 3;
        c.edit.keyDown(0x24,true);
        check(c.edit.caret == 0 && c.edit.selection == 3,"Shift Home retains the former endpoint");
        c.edit.keyDown(0x27,true);
        check(c.edit.caret == 1 && c.edit.selection == 2,"Shift Right changes the signed displacement");
        c.edit.keyDown(0x23,true);
        check(c.edit.caret == 4 && c.edit.selection == -1,"Shift End combines prior caret, displacement and current length");
        c.edit.keyDown(0x25,false);
        check(c.edit.caret == 3 && c.edit.selection == 0,"ordinary Left moves one unit and clears selection");
        c.events.clear();
        c.edit.keyDown('A',false);
        check(c.edit.caret == 3 && c.edit.selection == 0 && c.events.empty(),"unhandled keys add no inferred selection or clipboard feature");
        c.edit.keyDown(0x0d,false);
        check(c.events == std::vector<std::string>{"focus-clear","layout"},"Enter clears focus and lays out without confirming creation");
    }
    {
        Caption c; c.text = u"AB"; c.edit.caret = 0; c.edit.selection = 2;
        c.edit.keyDown(0x25,false);
        check(c.edit.caret == 0 && c.edit.selection == 2,"Left boundary does not clear an existing selection");
        c.edit.caret = 2; c.edit.selection = -2;
        c.edit.keyDown(0x27,false);
        check(c.edit.caret == 2 && c.edit.selection == -2,"Right boundary does not clear an existing selection");
        c.edit.caret = 0; c.edit.selection = 3;
        c.edit.clamp();
        check(c.edit.caret == -1 && c.edit.selection == 3,"ordered clamp's final correction writes caret, without iterative repair");
    }
    {
        plasma::Edit detached({}); detached.caret = 4; detached.selection = -2;
        detached.clamp(); detached.character('X'); detached.keyDown(0x0d,false);
        check(detached.caret == 0 && detached.selection == 0 && !detached.dirty,"missing caption clears indices only through clamp and ignores input");
    }
    {
        plx::Document doc;
        doc.shapes.push_back({"TextShape",{}});
        plasma::SceneNode root;
        const auto child = [&](plasma::SceneNode &parent,const wchar_t *text) {
            auto node = std::make_unique<plasma::SceneNode>();
            node->doc = &doc; node->shape = 0; node->text = text; node->parent = &parent;
            auto *result = node.get(); parent.children.push_back(std::move(node)); return result;
        };
        auto *empty = child(root,L"");
        child(*empty,L"nested");
        auto *first = child(root,L"first");
        child(root,L"last");
        check(plasma::captionText(*empty).empty() && plasma::captionText(root) == u"first",
              "own empty caption stops its subtree lookup; parent chooses first nonempty child result");
        first->visible = false; first->flags |= plx::NODE_DISABLED;
        check(plasma::captionText(root) == u"first","caption read has no independent visibility or disabled-frame gate");
        plasma::setCaptionText(root,u"updated");
        check(plasma::captionText(*empty) == u"updated" && plasma::captionText(*first) == u"first" &&
                  plasma::captionText(*empty->children[0]) == u"updated",
              "caption setter updates every admitted descendant and skips disabled subtrees");
        plasma::Edit editor({[&] {return plasma::captionText(*empty);},
                            [&](const auto &text) {plasma::setCaptionText(*empty,text);}});
        editor.caret = 2; editor.selection = 1;
        plasma::setCaptionText(*empty,u"ABCD");
        check(editor.caret == 2 && editor.selection == 1 && !editor.dirty,
              "external caption setter does not move the caret or synthesize an editing event");
        editor.character('X');
        check(plasma::captionText(*empty) == u"ABXD" && plasma::captionText(*empty->children[0]) == u"ABXD",
              "actual scene-caption ports publish editing through the existing recursive setter");
    }
    {
        bool available = false;
        std::u16string text = u"ABCD";
        plasma::Edit::Caption ports{[&] {return text;},[&](const auto &value) {text=value;}};
        ports.available = [&] {return available;};
        plasma::Edit edit(std::move(ports));
        edit.caret = 3; edit.selection = -2;
        edit.clamp();
        check(edit.caret == 0 && edit.selection == 0,"attachment with no cached caption clears both index words");
        available = true; edit.caret = 3; edit.selection = -2;
        edit.clamp();
        check(edit.caret == 3 && edit.selection == -2,"refreshing a valid caption preserves representable selection");
        available = false;
        edit.character('X'); edit.keyDown(0x08,false);
        check(text == u"ABCD" && edit.caret == 3 && edit.selection == -2 && !edit.dirty,
              "detached caption suppresses input before activity/index changes");
        edit.clamp();
        check(edit.caret == 0 && edit.selection == 0,"later attachment refresh performs its explicit missing-caption clamp");
    }
    {
        bool ownerActivity = false, validatorSawActivity = false;
        std::u16string text;
        plasma::Edit::Caption ports{[&] {return text;},[&](const auto &value) {text=value;}};
        ports.activity = [&] {ownerActivity=true;};
        ports.validate = [&](auto) {validatorSawActivity=ownerActivity; return true;};
        ports.changed = [&] {ownerActivity=false;};
        plasma::Edit edit(std::move(ports));
        edit.character('X');
        check(validatorSawActivity && !ownerActivity && edit.dirty && text == u"X",
              "activity write reaches the owner before validation and does not overwrite a later callback's clear");
        edit.keyDown('A',false);
        check(ownerActivity,"unhandled attached key still publishes its own immediate activity write");
    }
    std::printf("UTF16 caption editing: %s\n",failures ? "FAIL" : "PASS");
    return failures;
}
