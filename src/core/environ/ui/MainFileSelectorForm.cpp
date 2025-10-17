#include "MainFileSelectorForm.h"

#include <filesystem>

#include "cocos2d.h"
#include "cocostudio/CocoLoader.h"
#include "cocostudio/CCSSceneReader.h"
#include "Application.h"
#include "Platform.h"
#include "cocostudio/ActionTimeline/CCActionTimeline.h"
#include "ui/UIText.h"
#include "ui/UIHelper.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "cocos2d/MainScene.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "GlobalPreferenceForm.h"
#include "IndividualPreferenceForm.h"
#include "MessageBox.h"
#include "SimpleMediaFilePlayer.h"
#include "tinyxml2/tinyxml2.h"
#include "StorageImpl.h"
#include "TipsHelpForm.h"
#include "XP3RepackForm.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;

constexpr float UI_ACTION_DUR = 0.3f;
// const char *const FileName_NaviBar = "ui/NaviBarWithMenu.csb";
// const char *const FileName_Body = "ui/TableView.csb";
// const char * const FileName_BottomBar = "ui/BottomBar.csb";
// const char * const FileName_BtnPref = "ui/button/Pref.csb";
const char *const FileName_RecentPathListXML = "recentpath.xml";

bool TVPIsFirstLaunch = false;

std::deque<std::string> _HistoryPath;

static void _AskExit() {
    if(TVPShowSimpleMessageBoxYesNo(
           LocaleConfigManager::GetInstance()->GetText("sure_to_exit"),
           "Kirikiroid2") == 0)
        TVPExitApplication(0);
}

bool TVPCheckIsVideoFile(const char *uri);

static std::string _GetHistoryXMLPath() {
    return TVPGetInternalPreferencePath() + FileName_RecentPathListXML;
}

static void _LoadHistory() {
    std::string xmlpath = _GetHistoryXMLPath();
    tinyxml2::XMLDocument doc;
    if(!doc.LoadFile(xmlpath.c_str())) {
        tinyxml2::XMLElement *rootElement = doc.RootElement();
        if(rootElement) {
            for(tinyxml2::XMLElement *item =
                    rootElement->FirstChildElement("Item");
                item; item = item->NextSiblingElement("Item")) {
                const char *path = item->Attribute("Path");
                if(path) {
                    _HistoryPath.emplace_back(path);
                }
            }
        }
    } else {
        TVPIsFirstLaunch = true;
    }
}

static void _SaveHistory() {
    std::string xmlpath = _GetHistoryXMLPath();

    if(_HistoryPath.empty() && !FileUtils::getInstance()->isFileExist(xmlpath))
        return;

    tinyxml2::XMLDocument doc;
    doc.LinkEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *rootElement = doc.NewElement("RecentPathList");
    for(const std::string &path : _HistoryPath) {
        tinyxml2::XMLElement *item = doc.NewElement("Item");
        item->SetAttribute("Path", path.c_str());
        rootElement->LinkEndChild(item);
    }

    doc.LinkEndChild(rootElement);
    doc.SaveFile(xmlpath.c_str());
}

static void _RemoveHistory(const std::string &path) {
    auto it = std::find(_HistoryPath.begin(), _HistoryPath.end(), path);
    if(it != _HistoryPath.end()) {
        _HistoryPath.erase(it);
        _SaveHistory();
    }
}

static void _AddHistory(const std::string &path) {
    if(!_HistoryPath.empty() && _HistoryPath.front() == path)
        return;
    _RemoveHistory(path);
    _HistoryPath.emplace_front(path);
    _SaveHistory();
}

TVPMainFileSelectorForm::TVPMainFileSelectorForm() { _menu = nullptr; }

void TVPMainFileSelectorForm::onEnter() {
    inherit::onEnter();
    if(!filePath.empty()) {
        this->runFromPath(filePath);
        return;
    }
    if(_historyList) {
        _historyList->doLayout();
        auto &allcell = _historyList->getItems();
        for(Widget *cell : allcell) {
            static_cast<HistoryCell *>(cell)->rearrangeLayout();
        }
    }
}

void TVPMainFileSelectorForm::bindBodyController(const Node *allNodes) {
    TVPBaseFileSelectorForm::bindBodyController(allNodes);

    if(NaviBar.Right) {
        NaviBar.Right->addClickEventListener([this](Ref *r) { showMenu(r); });
    }
}

int TVPCheckArchive(const ttstr &localname);

void TVPMainFileSelectorForm::runFromPath(const std::string &path) {
    // int archiveType;
    if(checkStartupTjsScript(path)) {
        doStartup(path);
    } else if((/*archiveType = */ TVPCheckArchive(ttstr{ path })) == 1) {
        doStartup(path);
    } /*else if(archiveType == 0 && TVPCheckIsVideoFile(path.c_str())) {
        spdlog::info("Opening video file: {}", path);
        SimpleMediaFilePlayer *player = SimpleMediaFilePlayer::create();
        TVPMainScene::GetInstance()->addChild(player,
                                              10); // pushUIForm(player);
        player->PlayFile(path);
    }*/
}


void TVPMainFileSelectorForm::show() {
    ListHistory(); // filter history data

    bool first = true;
    std::string lastpath;
    if(!_HistoryPath.empty())
        lastpath = _HistoryPath.front();
    while(first ||
          (lastpath.size() > RootPathLen &&
           !FileUtils::getInstance()->isDirectoryExist(lastpath))) {
        first = false;
        std::pair<std::string, std::string> split_path = pathSplit(lastpath);
        if(split_path.second.empty()) {
            lastpath.clear();
            break;
        }
        lastpath = split_path.first;
    }
    if(lastpath.size() <= RootPathLen) {
        lastpath = TVPGetDriverPath()[0];
    }
    ListDir(lastpath);
}

static const std::string str_startup_tjs("startup.tjs");

bool TVPMainFileSelectorForm::checkStartupTjsScript(const std::string &path) {
#if _WIN32
    auto p =
        std::filesystem::u8path(path); // windows 上有兼容性问题必须使u8path
#else
    auto p = std::filesystem::path(path);
#endif
    if(std::filesystem::is_directory(p)) {
        return std::filesystem::exists(p / str_startup_tjs);
    }
    return false;
}

void TVPMainFileSelectorForm::onCellClicked(int idx) {
    FileInfo info = CurrentDirList[idx];
    TVPBaseFileSelectorForm::onCellClicked(idx);

    spdlog::debug("Selected file: {}, FullPath: {}", info.NameForDisplay,
                  info.FullPath);
    runFromPath(info.FullPath);
}

void TVPMainFileSelectorForm::getShortCutDirList(
    std::vector<std::string> &pathlist) {
    if(!_lastpath.empty()) {
        pathlist.emplace_back(_lastpath);
    }
    TVPBaseFileSelectorForm::getShortCutDirList(pathlist);
}

TVPMainFileSelectorForm *TVPMainFileSelectorForm::create() {
    auto *ret = new TVPMainFileSelectorForm();
    ret->autorelease();
    if(filePath.empty()) {
        ret->initFromWidget();
        ret->show();
    }
    return ret;
}

void TVPMainFileSelectorForm::initFromWidget() {
    _LoadHistory();
    setContentSize(TVPMainScene::GetInstance()->getUINodeSize());

    Node *root = Csd::createMainFileSelector(this->getContentSize(), 1);

    addChild(root);

    _fileList = root->getChildByName("fileList");
    _historyList = root->getChildByName<ListView *>("recentList");
    // TODO new node
    _fileOperateMenuNode = _historyList;
    inherit::initFromFile(Csd::createNaviBarWithMenu, Csd::createTableView,
                          Csd::createEmpty, _fileList);
}

void TVPMainFileSelectorForm::startup(const std::string &path) {
    //    if(TVPIsFirstLaunch) {
    //        TVPTipsHelpForm::show()->setOnExitCallback([this, path] {
    //            scheduleOnce([this, path](float) { doStartup(path); }, 0,
    //                         "startup");
    //        });
    //    } else {
    doStartup(path);
    //    }
}

void TVPMainFileSelectorForm::doStartup(const std::string &path) {
    if(TVPMainScene::GetInstance()->startupFrom(path)) {
        if(GlobalConfigManager::GetInstance()->GetValue<bool>(
               "remember_last_path", true)) {
            _AddHistory(path);
        }
    }
}

std::string TVPGetOpenGLInfo();

void TVPOpenPatchLibUrl();

void TVPMainFileSelectorForm::showMenu(Ref *) {
    if(!_menu) {
        cocos2d::Size uiSize = getContentSize();
        CSBReader reader;
        _menu = reader.Load("ui/MenuList.csb");
        _menu->setAnchorPoint(Vec2::ZERO);
        _menu->setPosition(Vec2(uiSize.width, 0));
        _mask = LayerColor::create(Color4B::BLACK, uiSize.width, uiSize.height);
        _mask->setOpacity(0);
        _touchHideMenu = ui::Widget::create();
        _touchHideMenu->setAnchorPoint(Vec2::ZERO);
        _touchHideMenu->setContentSize(uiSize);
        _touchHideMenu->addClickEventListener([this](Ref *) {
            if(isMenuShowed())
                hideMenu(nullptr);
        });
        _mask->addChild(_touchHideMenu);
        addChild(_mask);
        addChild(_menu);
        if(uiSize.width > uiSize.height) {
            uiSize.width /= 3;
        } else {
            uiSize.width *= 0.6f;
        }
        cocos2d::Size menuSize = _menu->getContentSize();
        float scale = uiSize.width / menuSize.width;
        menuSize.height = uiSize.height / scale;
        _menu->setScale(scale);
        _menu->setContentSize(menuSize);
        ui::Helper::doLayout(_menu);

        newLocalPref = reader.findController("newLocalPref");
        localPref = reader.findController("localPref");
        sizeNewLocalPref = newLocalPref->getContentSize();
        sizeLocalPref = localPref->getContentSize();

        _menuList =
            static_cast<ui::ListView *>(reader.findController("menulist"));

        // captions
        LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
        localeMgr->initText(reader.findController<Text>("titleRotate"));
        localeMgr->initText(reader.findController<Text>("titleGlobalPref"));
        localeMgr->initText(reader.findController<Text>("titleNewLocalPref"));
        localeMgr->initText(reader.findController<Text>("titleLocalPref"));
        localeMgr->initText(reader.findController<Text>("titleHelp"));
        localeMgr->initText(reader.findController<Text>("titleAbout"));
        localeMgr->initText(reader.findController<Text>("titleExit"));
        localeMgr->initText(reader.findController<Text>("titleRepack"));
        localeMgr->initText(reader.findController<Text>("titleNewFolder"));

        // button events
        reader.findWidget("btnRotate")->addClickEventListener([](Ref *) {
            TVPMainScene::GetInstance()->pushUIForm(
                TVPGlobalPreferenceForm::create());
        });
        reader.findWidget("btnGlobalPref")->addClickEventListener([](Ref *) {
            TVPMainScene::GetInstance()->pushUIForm(
                TVPGlobalPreferenceForm::create());
        });
        reader.findWidget("btnNewLocalPref")
            ->addClickEventListener([this](Ref *) {
                if(IndividualConfigManager::GetInstance()->CreatePreferenceAt(
                       CurrentPath)) {
                    TVPMainScene::GetInstance()->pushUIForm(
                        IndividualPreferenceForm::create());
                    hideMenu(nullptr);
                }
            });
        reader.findWidget("btnLocalPref")->addClickEventListener([this](Ref *) {
            onShowPreferenceConfigAt(CurrentPath);
        });
        reader.findWidget("btnHelp")->addClickEventListener(
            [this](Ref *) { TVPTipsHelpForm::show(); });

        reader.findWidget("btnAbout")->addClickEventListener([](Ref *) {
            std::string versionText = "Version ";
            versionText += TVPGetPackageVersionString();
            versionText += "\n";
            versionText +=
                LocaleConfigManager::GetInstance()->GetText("about_content");

            const char *pszBtnText[] = {
                LocaleConfigManager::GetInstance()->GetText("ok").c_str(),
                LocaleConfigManager::GetInstance()
                    ->GetText("browse_patch_lib")
                    .c_str(),
                LocaleConfigManager::GetInstance()
                    ->GetText("device_info")
                    .c_str(),
            };

            std::string strCaption =
                LocaleConfigManager::GetInstance()->GetText("menu_about");
            int n = TVPShowSimpleMessageBox(
                versionText.c_str(), strCaption.c_str(),
                sizeof(pszBtnText) / sizeof(pszBtnText[0]), pszBtnText);

            switch(n) {
                case 1:
                    TVPOpenPatchLibUrl();
                    break;
                case 2:
                    cocos2d::Director::getInstance()
                        ->getScheduler()
                        ->performFunctionInCocosThread([] {
                            std::string text = TVPGetOpenGLInfo();
                            const char *pOK = LocaleConfigManager::GetInstance()
                                                  ->GetText("ok")
                                                  .c_str();
                            TVPShowSimpleMessageBox(
                                text.c_str(),
                                LocaleConfigManager::GetInstance()
                                    ->GetText("device_info")
                                    .c_str(),
                                1, &pOK);
                        });
                    break;
            }
        });
        reader.findWidget("btnExit")->addClickEventListener(
            [](Ref *) { _AskExit(); });
        // TODO
        //		reader.findWidget("btnRepack")->addClickEventListener([this](Ref*)
        //{ 			TVPProcessXP3Repack(CurrentPath);
        // hideMenu(nullptr);
        //		});
        reader.findWidget("btnNewFolder")->addClickEventListener([this](Ref *) {
            ttstr name = TJS_W("New Folder");
            std::vector<ttstr> btns;
            btns.emplace_back("OK");
            btns.emplace_back("Cancel");
            if(TVPShowSimpleInputBox(name, "Input name", "", btns) == 0) {
                ttstr newname(CurrentPath);
                newname += TJS_W("/");
                newname += name;
                if(!TVPCreateFolders(newname)) {
                    TVPShowSimpleMessageBox(TJS_W("Fail to create folder."),
                                            TJS_W("Error"));
                } else {
                    ListDir(CurrentPath);
                }
            }
            hideMenu(nullptr);
        });
    }
    const cocos2d::Size &uiSize = getContentSize();
    const Vec2 &pos = _menu->getPosition();
    const cocos2d::Size &size = _menu->getContentSize();
    float w = size.width * _menu->getScale();
    if(pos.x > uiSize.width - w / 10.0f) {
        if(IndividualConfigManager::CheckExistAt(CurrentPath)) {
            localPref->setVisible(true);
            localPref->setContentSize(sizeLocalPref);
            newLocalPref->setVisible(false);
            newLocalPref->setContentSize(cocos2d::Size::ZERO);
        } else {
            newLocalPref->setVisible(true);
            newLocalPref->setContentSize(sizeNewLocalPref);
            localPref->setVisible(false);
            localPref->setContentSize(cocos2d::Size::ZERO);
        }
        _menuList->requestDoLayout();
        _mask->stopAllActions();
        _mask->runAction(FadeTo::create(UI_ACTION_DUR, 128));
        _menu->stopAllActions();
        _menu->runAction(EaseQuadraticActionOut::create(
            MoveTo::create(UI_ACTION_DUR, Vec2(uiSize.width - w, pos.y))));
        _touchHideMenu->setTouchEnabled(true);
    }
}

void TVPMainFileSelectorForm::hideMenu(cocos2d::Ref *) {
    if(!_menu)
        return;
    _mask->stopAllActions();
    _mask->runAction(FadeOut::create(UI_ACTION_DUR));
    _menu->stopAllActions();
    _menu->runAction(EaseQuadraticActionOut::create(MoveTo::create(
        UI_ACTION_DUR, Vec2(getContentSize().width, _menu->getPositionY()))));
    _touchHideMenu->setTouchEnabled(false);
}

bool TVPMainFileSelectorForm::isMenuShowed() {
    if(!_menu)
        return false;
    const Vec2 &pos = _menu->getPosition();
    const cocos2d::Size &size = _menu->getContentSize();
    float w = size.width * _menu->getScale();
    if(pos.x < getContentSize().width - w * 0.9f) {
        return true;
    }
    return false;
}

bool TVPMainFileSelectorForm::isMenuShrinked() {
    if(!_menu)
        return true;
    const Vec2 &pos = _menu->getPosition();
    const cocos2d::Size &size = _menu->getContentSize();
    float w = size.width * _menu->getScale();
    if(pos.x > getContentSize().width - w / 10.0f) {
        return false;
    }
    return true;
}

void TVPMainFileSelectorForm::onShowPreferenceConfigAt(
    const std::string &path) {
    if(IndividualConfigManager::GetInstance()->UsePreferenceAt(path)) {
        TVPMainScene::GetInstance()->pushUIForm(
            IndividualPreferenceForm::create());
    }
}

void TVPMainFileSelectorForm::ListHistory() {
    if(!_historyList)
        return;

    _historyList->removeAllChildren();

    auto createEmptyCell = [this]() -> HistoryCell * {
        auto *cell = new HistoryCell();
        cell->init();
        cocos2d::Size cellSize = _historyList->getContentSize();
        cellSize.height = 100;
        cell->setContentSize(cellSize);
        return cell;
    };

    // 添加顶部空白 cell
    _historyList->pushBackCustomItem(createEmptyCell());

    for(auto it = _HistoryPath.begin(); it != _HistoryPath.end();) {
        const std::string &fullPath = *it;

        if(!(TVPCheckExistentLocalFile(fullPath) ||
             TVPCheckExistentLocalFolder(fullPath))) {
            it = _HistoryPath.erase(it);
            continue;
        }

        auto fullPathSpt = pathSplit(fullPath);
        auto parentPath = fullPathSpt.first;
        auto lastName = fullPathSpt.second;

        auto parentPathSpt = pathSplit(parentPath);
        auto grandParentPath = parentPathSpt.first;
        auto parentName = parentPathSpt.second;

        HistoryCell *cell = HistoryCell::create(fullPath, grandParentPath + "/",
                                                parentName, "/" + lastName);

        Widget::ccWidgetClickCallback funcConf;
        if(TVPCheckExistentLocalFile(parentPath +
                                     "/Kirikiroid2Preference.xml")) {
            funcConf = [this, parentPath](Ref *) {
                onShowPreferenceConfigAt(parentPath);
            };
        }

        cell->initFunction(
            [this, cell](Ref *ref) { RemoveHistoryCell(ref, cell); },
            [this, parentPath](Ref *) { ListDir(parentPath); }, funcConf,
            [this, fullPath](Ref *) { startup(fullPath); });

        cocos2d::Size cellSize = cell->getContentSize();
        cellSize.width = _historyList->getContentSize().width;
        cell->setContentSize(cellSize);

        _historyList->pushBackCustomItem(cell);
        ++it;
    }

    // 添加底部空白 cell
    _historyList->pushBackCustomItem(createEmptyCell());
}

void TVPMainFileSelectorForm::RemoveHistoryCell(cocos2d::Ref *btn,
                                                HistoryCell *cell) {
    static_cast<Widget *>(btn)->setEnabled(false);
    cell->runAction(Sequence::createWithTwoActions(
        EaseQuadraticActionOut::create(
            MoveBy::create(0.25, Vec2(-cell->getContentSize().width, 0))),
        CallFuncN::create([this](Node *p) {
            auto *cell = static_cast<HistoryCell *>(p);
            ssize_t idx = _historyList->getIndex(cell);
            if(idx < 0)
                return;
            _historyList->removeItem(idx);
        })));
    _RemoveHistory(cell->getFullpath());
    _SaveHistory();
}

void TVPMainFileSelectorForm::onKeyPressed(
    cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event) {
    if(keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
        if(isMenuShowed()) {
            hideMenu(nullptr);
        } else {
            _AskExit();
        }
    } else if(keyCode == EventKeyboard::KeyCode::KEY_MENU) {
        if(isMenuShrinked()) {
            showMenu(nullptr);
        }
    } else {
        inherit::onKeyPressed(keyCode, event);
    }
}

void TVPMainFileSelectorForm::HistoryCell::initInfo(
    const std::string &fullpath, const std::string &prefix,
    const std::string &pathname, const std::string &filename) {
    _fullpath = fullpath;

    CSBReader reader;
    _root = reader.Load("ui/RecentListItem.csb");
    _scrollview = static_cast<cocos2d::ui::ScrollView *>(
        reader.findController("scrollview"));
    _btn_delete =
        static_cast<cocos2d::ui::Widget *>(reader.findController("btn_delete"));
    _btn_jump =
        static_cast<cocos2d::ui::Widget *>(reader.findController("btn_jump"));
    _btn_conf =
        static_cast<cocos2d::ui::Widget *>(reader.findController("btn_conf"));
    _btn_play =
        static_cast<cocos2d::ui::Widget *>(reader.findController("btn_play"));
    _prefix = static_cast<cocos2d::ui::Text *>(reader.findController("prefix"));
    _path = static_cast<cocos2d::ui::Text *>(reader.findController("path"));
    _file = static_cast<cocos2d::ui::Text *>(reader.findController("file"));
    _panel_delete = reader.findController("panel_delete");
    if(!_panel_delete)
        _panel_delete = _btn_delete;
    _scrollview->setScrollBarEnabled(false);
    _scrollview->setPropagateTouchEvents(true);

    _prefix->setString(prefix);
    _path->setString(pathname);
    _file->setString(filename);

    setContentSize(_root->getContentSize());
    addChild(_root);
}

void TVPMainFileSelectorForm::HistoryCell::rearrangeLayout() {
    if(!_root)
        return;
    _root->setContentSize(this->getContentSize());
    ui::Helper::doLayout(_root);
    Vec2 pt = Vec2::ZERO;
    pt.x = _file->getContentSize().width;
    Vec2 ptWorld = _file->convertToWorldSpace(pt);
    cocos2d::Size viewSize = _scrollview->getContentSize();
    Node *container = _scrollview->getInnerContainer();
    pt = container->convertToNodeSpace(ptWorld);
    float btnw = _panel_delete->getContentSize().width;
    float offsetx = 0;
    if(pt.x > viewSize.width) {
        float neww = pt.x;
        pt.y = 0;
        pt.x = _path->getContentSize().width;
        ptWorld = _path->convertToWorldSpace(pt);
        pt = container->convertToNodeSpace(ptWorld);
        if(pt.x > viewSize.width) {
            offsetx = viewSize.width - pt.x;
        }
        viewSize.width = neww;
    }
    _panel_delete->setPositionX(viewSize.width + btnw);
    viewSize.width += btnw + btnw;
    _scrollview->setInnerContainerSize(viewSize);
    container->setPosition(offsetx, 0);
}

void TVPMainFileSelectorForm::HistoryCell::initFunction(
    const ccWidgetClickCallback &funcDel, const ccWidgetClickCallback &funcJump,
    const ccWidgetClickCallback &funcConf,
    const ccWidgetClickCallback &funcPlay) {
    _btn_delete->addClickEventListener(funcDel);
    _btn_play->addClickEventListener(funcPlay);
    if(funcConf)
        _btn_conf->addClickEventListener(funcConf);
    else
        _btn_conf->setVisible(false);
    _btn_jump->addClickEventListener(funcJump);
}

void TVPMainFileSelectorForm::HistoryCell::onSizeChanged() {}
