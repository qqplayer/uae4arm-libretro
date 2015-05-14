#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"


#define MAX_HD_DEVICES 5
enum { COL_DEVICE, COL_VOLUME, COL_PATH, COL_READWRITE, COL_SIZE, COL_BOOTPRI, COL_COUNT };

static const char *column_caption[] = {
  "Device", "Volume", "Path", "R/W", "Size", "Bootpri" };
static const int COLUMN_SIZE[] = {
   50,  // Device
   70,  // Volume
  260,  // Path
   40,  // R/W
   50,  // Size
   50   // Bootpri
};

static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_HD_DEVICES];
static gcn::TextField* listCells[MAX_HD_DEVICES][COL_COUNT];
static gcn::Button* listCmdProps[MAX_HD_DEVICES];
static gcn::ImageButton* listCmdDelete[MAX_HD_DEVICES];
static gcn::Button* cmdAddDirectory;
static gcn::Button* cmdAddHardfile;


class HDRemoveActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      for(int i=0; i<MAX_HD_DEVICES; ++i)
      {
        if (actionEvent.getSource() == listCmdDelete[i])
        {
          kill_filesys_unit(currprefs.mountinfo, i);
          break;
        }
      }
      cmdAddDirectory->requestFocus();
      RefreshPanelHD();
    }
};
static HDRemoveActionListener* hdRemoveActionListener;


class HDEditActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      for(int i=0; i<MAX_HD_DEVICES; ++i)
      {
        if (actionEvent.getSource() == listCmdProps[i])
        {
          int type = is_hardfile (currprefs.mountinfo, i);
          if(type == FILESYS_VIRTUAL)
            EditFilesysVirtual(i);
          else
            EditFilesysHardfile(i);
          listCmdProps[i]->requestFocus();
          break;
        }
      }
      RefreshPanelHD();
    }
};
static HDEditActionListener* hdEditActionListener;


class AddVirtualHDActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      EditFilesysVirtual(-1);
      cmdAddDirectory->requestFocus();
      RefreshPanelHD();
    }
};
AddVirtualHDActionListener* addVirtualHDActionListener;


class AddHardfileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      EditFilesysHardfile(-1);
      cmdAddHardfile->requestFocus();
      RefreshPanelHD();
    }
};
AddHardfileActionListener* addHardfileActionListener;


void InitPanelHD(const struct _ConfigCategory& category)
{
  int row, col;
  int posX;
  int posY = DISTANCE_BORDER;
  char tmp[20];
  
  hdRemoveActionListener = new HDRemoveActionListener();
  hdEditActionListener = new HDEditActionListener();
  addVirtualHDActionListener = new AddVirtualHDActionListener();
  addHardfileActionListener = new AddHardfileActionListener();
  
  for(col=0; col<COL_COUNT; ++col)
    lblList[col] = new gcn::Label(column_caption[col]);

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    listEntry[row] = new gcn::Container();
    listEntry[row]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, TEXTFIELD_HEIGHT + 4);
    listEntry[row]->setBaseColor(gui_baseCol);
    listEntry[row]->setFrameSize(0);
    
    listCmdProps[row] = new gcn::Button("...");
    listCmdProps[row]->setBaseColor(gui_baseCol);
    listCmdProps[row]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
    snprintf(tmp, 20, "cmdProp%d", row);
    listCmdProps[row]->setId(tmp);
    listCmdProps[row]->addActionListener(hdEditActionListener);
    
    listCmdDelete[row] = new gcn::ImageButton("data/delete.png");
    listCmdDelete[row]->setBaseColor(gui_baseCol);
    listCmdDelete[row]->setSize(SMALL_BUTTON_HEIGHT, SMALL_BUTTON_HEIGHT);
    snprintf(tmp, 20, "cmdDel%d", row);
    listCmdDelete[row]->setId(tmp);
    listCmdDelete[row]->addActionListener(hdRemoveActionListener);
    
    for(col=0; col<COL_COUNT; ++col)
    {
      listCells[row][col] = new gcn::TextField();
      listCells[row][col]->setSize(COLUMN_SIZE[col] - 8, TEXTFIELD_HEIGHT);
      listCells[row][col]->setEnabled(false);
      listCells[row][col]->setBackgroundColor(gui_baseCol);
    }
  }
  
  cmdAddDirectory = new gcn::Button("Add Directory");
  cmdAddDirectory->setBaseColor(gui_baseCol);
  cmdAddDirectory->setSize(BUTTON_WIDTH + 20, BUTTON_HEIGHT);
  cmdAddDirectory->setId("cmdAddDir");
  cmdAddDirectory->addActionListener(addVirtualHDActionListener);
  
  cmdAddHardfile = new gcn::Button("Add Hardfile");
  cmdAddHardfile->setBaseColor(gui_baseCol);
  cmdAddHardfile->setSize(BUTTON_WIDTH + 20, BUTTON_HEIGHT);
  cmdAddHardfile->setId("cmdAddHDF");
  cmdAddHardfile->addActionListener(addHardfileActionListener);
  
  posX = DISTANCE_BORDER + 2 + SMALL_BUTTON_WIDTH + 34;
  for(col=0; col<COL_COUNT; ++col)
  {
    category.panel->add(lblList[col], posX, posY);
    posX += COLUMN_SIZE[col];
  }
  posY += lblList[0]->getHeight() + 2;
  
  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    posX = 0;
    listEntry[row]->add(listCmdProps[row], posX, 2);
    posX += listCmdProps[row]->getWidth() + 4;
    listEntry[row]->add(listCmdDelete[row], posX, 2);
    posX += listCmdDelete[row]->getWidth() + 8;
    for(col=0; col<COL_COUNT; ++col)
    {
      listEntry[row]->add(listCells[row][col], posX, 2);
      posX += COLUMN_SIZE[col];
    }
    category.panel->add(listEntry[row], DISTANCE_BORDER, posY);
    posY += listEntry[row]->getHeight() + 4;
  }
  
  posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
  category.panel->add(cmdAddDirectory, DISTANCE_BORDER, posY);
  category.panel->add(cmdAddHardfile, DISTANCE_BORDER + cmdAddDirectory->getWidth() + DISTANCE_NEXT_X, posY);
  
  RefreshPanelHD();
}


void ExitPanelHD(void)
{
  int row, col;

  for(col=0; col<COL_COUNT; ++col)
    delete lblList[col];

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    delete listCmdProps[row];
    delete listCmdDelete[row];
    for(col=0; col<COL_COUNT; ++col)
      delete listCells[row][col];
    delete listEntry[row];
  }
  
  delete cmdAddDirectory;
  delete cmdAddHardfile;
  
  delete hdRemoveActionListener;
  delete hdEditActionListener;
  delete addVirtualHDActionListener;
  delete addHardfileActionListener;
}


void RefreshPanelHD(void)
{
  int row, col;
  char tmp[32];
  char *volname, *devname, *rootdir, *filesys;
  int secspertrack, surfaces, cylinders, reserved, blocksize, readonly, type, bootpri;
  uae_u64 size;
  const char *failure;
  int units = nr_units(currprefs.mountinfo);
  
  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    if(row < units)
    {
      failure = get_filesys_unit(currprefs.mountinfo, row, 
        &devname, &volname, &rootdir, &readonly, &secspertrack, &surfaces, &reserved, 
        &cylinders, &size, &blocksize, &bootpri, &filesys, 0);
      type = is_hardfile (currprefs.mountinfo, row);
      
      if(type == FILESYS_VIRTUAL)
      {
        listCells[row][COL_DEVICE]->setText(devname);
        listCells[row][COL_VOLUME]->setText(volname);
        listCells[row][COL_PATH]->setText(rootdir);
        if(readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
        listCells[row][COL_SIZE]->setText("n/a");
        snprintf(tmp, 32, "%d", bootpri);
        listCells[row][COL_BOOTPRI]->setText(tmp);
      }
      else
      {
        listCells[row][COL_DEVICE]->setText(devname);
        listCells[row][COL_VOLUME]->setText("n/a");
        listCells[row][COL_PATH]->setText(rootdir);
        if(readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
  	    if (size >= 1024 * 1024 * 1024)
	        snprintf (tmp, 32, "%.1fG", ((double)(uae_u32)(size / (1024 * 1024))) / 1024.0);
  	    else
	        snprintf (tmp, 32, "%.1fM", ((double)(uae_u32)(size / (1024))) / 1024.0);
        listCells[row][COL_SIZE]->setText(tmp);
        snprintf(tmp, 32, "%d", bootpri);
        listCells[row][COL_BOOTPRI]->setText(tmp);
      }
      listCmdProps[row]->setEnabled(true);
      listCmdDelete[row]->setEnabled(true);
    }
    else
    {
      // Empty slot
      for(col=0; col<COL_COUNT; ++col)
        listCells[row][col]->setText("");
      listCmdProps[row]->setEnabled(false);
      listCmdDelete[row]->setEnabled(false);
    }
  }
}