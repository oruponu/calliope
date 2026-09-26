#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <optional>
#include <utility>

template <class Editor, class Draft> class CalloutEditorSession
{
public:
    CalloutEditorSession() = default;
    CalloutEditorSession(const CalloutEditorSession&) = delete;
    CalloutEditorSession& operator=(const CalloutEditorSession&) = delete;
    ~CalloutEditorSession() { close(); }

    bool isOpen() const { return draft.has_value(); }
    Draft* current() { return draft ? &*draft : nullptr; }
    const Draft* current() const { return draft ? &*draft : nullptr; }

    void open(Draft initial, std::unique_ptr<Editor> content, juce::Rectangle<int> screenArea)
    {
        draft = std::move(initial);
        editor = content.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(content), screenArea, nullptr);
        box.setDismissalMouseClicksAreAlwaysConsumed(true);
        callout = &box;
    }

    // The editor dismisses its own CallOutBox, so don't dismiss it here.
    Draft finish()
    {
        jassert(draft.has_value());
        Draft finished = std::move(*draft);
        reset();
        return finished;
    }

    void close()
    {
        if (editor != nullptr)
            editor->abandon();
        if (callout != nullptr)
            callout->dismiss();
        reset();
    }

private:
    void reset()
    {
        draft.reset();
        editor = nullptr;
        callout = nullptr;
    }

    std::optional<Draft> draft;
    juce::Component::SafePointer<juce::CallOutBox> callout;
    juce::Component::SafePointer<Editor> editor;
};
