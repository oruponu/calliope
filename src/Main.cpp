#include "MainComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class CalliopeApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Calliope"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override { mainWindow = std::make_unique<MainWindow>(getApplicationName()); }

    void shutdown() override { mainWindow.reset(); }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(
                  name,
                  juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
            setFullScreen(true);
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }

        void activeWindowStatusChanged() override
        {
            if (isActiveWindow())
            {
                auto safeThis = juce::Component::SafePointer<MainWindow>(this);
                juce::MessageManager::callAsync(
                    [safeThis]()
                    {
                        if (safeThis != nullptr)
                            if (auto* content = safeThis->getContentComponent())
                                content->grabKeyboardFocus();
                    });
            }
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(CalliopeApplication)
