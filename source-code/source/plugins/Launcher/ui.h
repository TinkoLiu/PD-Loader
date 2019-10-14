﻿#pragma once
#include "framework.h"
#include <msclr\marshal_cppstd.h>
#include <cstdlib>
#include "TabPadding.h"
#include <GL\freeglut.h>
#include <GL\GL.h>
#include "SkinnedMessageBox.h"
#include "PluginConfig.h"
#include "GPUModel.h"

namespace Launcher {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace msclr::interop;

	/// <summary>
	/// Summary for ui
	/// </summary>
	public ref class ui : public System::Windows::Forms::Form
	{
	public:
		ui(void)
		{
			InitializeComponent();
			this->button_Discord->Cursor = Cursors::Hand;
			this->ClientSize = Drawing::Size(444, 323);
			TabPadding^ tabpad = gcnew TabPadding(tabControl);


			// if components.ini has no section name, add one
			if (!IsLineInFile("[components]", COMPONENTS_FILE))
			{
				PrependFile("[components]\n", COMPONENTS_FILE);
			}

			// if playerdata.ini has no section name, add one
			if (!IsLineInFile("[playerdata]", PLAYERDATA_FILE))
			{
				PrependFile("[playerdata]\n", PLAYERDATA_FILE);
			}

			// if keyconfig.ini has no section name, add one
			if (!IsLineInFile("[keyconfig]", KEYCONFIG_FILE))
			{
				PrependFile("[keyconfig]\n", KEYCONFIG_FILE);
			}


			this->panel_ScreenRes->SuspendLayout();

			// populate options (patches) from array in framework
			// (easier than manually setting everything up)
			int screenresY = 3;
			for (ConfigOptionBase* option : screenResolutionArray)
			{
				option->hasChanged = ResolutionConfigChanged;
				screenresY += option->AddToPanel(panel_ScreenRes, 12, screenresY, toolTip1);
			}
			((ComboBox^)ComboBox::FromHandle(DisplayModeDropdown->mainControlHandle))->SelectedIndexChanged += gcnew System::EventHandler(this, &ui::DisplayTypeChangedHandler);
			DisplayTypeChangedHandler(this, gcnew EventArgs); // run handler now
			this->panel_ScreenRes->ResumeLayout(false);
			this->panel_ScreenRes->PerformLayout();


			this->panel_IntRes->SuspendLayout();

			// populate options (patches) from array in framework
			// (easier than manually setting everything up)
			int intresY = 3;
			for (ConfigOptionBase* option : internalResolutionArray)
			{
				option->hasChanged = ResolutionConfigChanged;
				intresY += option->AddToPanel(panel_IntRes, 12, intresY, toolTip1);
			}
			((CheckBox^)CheckBox::FromHandle(InternalResolutionCheckbox->mainControlHandle))->CheckedChanged += gcnew System::EventHandler(this, &ui::InternalResEnabledChangedHandler);
			InternalResEnabledChangedHandler(this, gcnew EventArgs); // run handler now
			this->panel_IntRes->ResumeLayout(false);
			this->panel_IntRes->PerformLayout();


			this->panel_Patches->SuspendLayout();

			// populate options (patches) from array in framework
			// (easier than manually setting everything up)
			int optionsY = 3;
			for (ConfigOptionBase* option : optionsArray)
			{
				option->hasChanged = OptionsConfigChanged;
				optionsY += option->AddToPanel(panel_Patches, 12, optionsY, toolTip1);
			}
			this->panel_Patches->ResumeLayout(false);
			this->panel_Patches->PerformLayout();



			this->panel_Playerdata->SuspendLayout();

			// populate playerdata options from array in framework
			// (easier than manually setting everything up)
			int playerdataY = 3;
			for (ConfigOptionBase* option : playerdataArray)
			{
				option->hasChanged = PlayerdataConfigChanged;
				playerdataY += option->AddToPanel(panel_Playerdata, 12, playerdataY, toolTip1);
			}
			this->panel_Playerdata->ResumeLayout(false);
			this->panel_Playerdata->PerformLayout();


			this->panel_Components->SuspendLayout();

			// populate components from array in framework
			// (easier than manually setting everything up)
			int componentsY = 3;
			for (ConfigOptionBase* component : componentsArray)
			{
				component->hasChanged = ComponentsConfigChanged;
				componentsY += component->AddToPanel(panel_Components, 12, componentsY, toolTip1);
			}
			this->panel_Components->ResumeLayout(false);
			this->panel_Components->PerformLayout();


			AllPlugins = LoadPlugins();
			AllPluginOpts = GetPluginOptions(&AllPlugins);

			this->panel_Plugins->SuspendLayout();

			int pluginsY = 3;
			for (ConfigOptionBase* option : AllPluginOpts)
			{
				option->hasChanged = new bool(false);
				pluginsY += option->AddToPanel(panel_Plugins, 12, pluginsY, toolTip1);
			}
			this->panel_Plugins->ResumeLayout(false);
			this->panel_Plugins->PerformLayout();


			// trick Optimus into switching to the NVIDIA GPU
			if (cuInit != NULL) cuInit(0);

			int argc = 1;
			char* argv[1] = { (char*)"" };

			glutInit(&argc, argv);
			int window = glutCreateWindow("glut"); // a context must be created to use glGetString

			String^ vendor = gcnew String((char*)glGetString(GL_VENDOR));
			vendor = vendor->Replace(" Corporation", ""); // this is useless..  remove it to help ensure the GPU type line fits
			String^ renderer = gcnew String((char*)glGetString(GL_RENDERER));
			String^ version = gcnew String((char*)glGetString(GL_VERSION));

			String^ gpuModel = gcnew String(GPUModel::getGpuName().c_str());

			String^ wineVersion = gcnew String(GPUModel::getWineVer().c_str());

			glutDestroyWindow(window); // destroy the window so it doesn't remain on screen
			if (glutMainLoopEventDynamic != NULL) glutMainLoopEventDynamic(); // freeglut needs this

			if (wineVersion != "")
			{
				this->labelGPU->Text = "Wine " + wineVersion + " (at least 4.12.1 is required)\n";
			}
			else this->labelGPU->Text = "GPU Info:\n";
			this->labelGPU->Text += vendor + " " + renderer + "\n";
			this->labelGPU->Text += "OpenGL: " + version + "\n";

			int linkStart = this->labelGPU->Text->Length;
			bool showGpuDialog = false;
			if (!vendor->Contains("NVIDIA")) // check OpenGL renderer to get actual GPU being used for vendor check (ensure not running on iGPU)
			{
				this->labelGPU->Text += "Issues: NVIDIA GPU REQUIRED!";
				GPUIssueText = "Your graphics card is not supported! Only NVIDIA GPUs can run the game.\nPlease use a GTX 600 series or later GPU.\n\nIf you have a laptop with an NVIDIA GPU, you may need to set diva.exe to use it in NVIDIA Control Panel.";
				this->labelGPU->LinkColor = System::Drawing::Color::Red;
				showGpuDialog = true;
			}
			else if (gpuModel->StartsWith("TU"))
			{
				this->labelGPU->Text += "Issues: Turing GPU detected! Possible noise.\n(Click for more information)";
				GPUIssueText = "On Turing GPUs (GTX 16xx/RTX 20xx), some important character shaders have issues resulting in lines/noise.\nPlease make sure the ShaderPatch plugin is enabled.";
				this->labelGPU->LinkColor = System::Drawing::Color::Yellow;
			}
			else if (gpuModel->StartsWith("GM") || gpuModel->StartsWith("GP") || gpuModel->StartsWith("GV"))
			{
				this->labelGPU->Text += "Issues: May have minor noise on some stages.\n(Click for more information)";
				GPUIssueText = "On Maxwell GPUs (~GTX 900) or newer, some minor stage shaders create noise.\nPlease make sure the ShaderPatch plugin is enabled.";
				this->labelGPU->LinkColor = System::Drawing::Color::Yellow;
			}
			else if (gpuModel->StartsWith("GK"))
			{
				this->labelGPU->Text += "Issues: None.";
				GPUIssueText = "Your GPU should have no issues running the game.";
				this->labelGPU->LinkColor = System::Drawing::Color::Lime;
			}
			else if (gpuModel->StartsWith("GF"))
			{
				if (version[0] < '4')
				{
					this->labelGPU->Text += "Issues: Driver too old.";
					GPUIssueText = "Your GPU should be able to run the game, but it looks like your OpenGL version is too old.\nA driver update should fix this.";
					this->labelGPU->LinkColor = System::Drawing::Color::Orange;
					showGpuDialog = true;
				}
				else
				{
					this->labelGPU->Text += "Issues: No known issues.";
					GPUIssueText = "Your GPU should have no issues running the game, but it is older than the GPU the game was originally designed for (GTX 650 Ti).\nThere may be some issues with graphics.\nPlease report any issues so that they can be analysed and potentially fixed.";
					this->labelGPU->LinkColor = System::Drawing::Color::Lime;
				}
			}
			else if (version[0] >= '4' && gpuModel->Length > 0 && !gpuModel->StartsWith("Unk") && !gpuModel->StartsWith("Oth"))
			{
				this->labelGPU->Text += "Issues: GPU may be too new.\n(Click for more information)";
				GPUIssueText = "It looks like your GPU may be too new. Only up to Turing (GTX 16xx/RTX 20xx) is currently supported.\nGood news: the game should be capable of running, but shader patches (probably needed) may not support it yet.\nYou will likely see lines/noise on important character shaders and some minor stage shaders.\nPlease report any issues so that ShaderPatch can be updated.";
				this->labelGPU->LinkColor = System::Drawing::Color::Orange;
			}
			else if (gpuModel->StartsWith("G") || gpuModel->StartsWith("NV") || gpuModel->StartsWith("NB") || gpuModel->StartsWith("N10") || gpuModel->StartsWith("MCP"))
			{
				this->labelGPU->Text += "Issues: GPU too old! 3D rendering might be broken.\n(Click for more information)";
				GPUIssueText = "Your GPU is very old and does not support rendering techniques used by the game.\nYou may be able to play, but graphics will likely have major issues.\nPlease upgrade to a GTX 600 series or later GPU.";
				this->labelGPU->LinkColor = System::Drawing::Color::Orange;
				showGpuDialog = true;
			}
			else //if (gpuModel->Length == 0 || gpuModel->StartsWith("Unk") || gpuModel->StartsWith("Oth"))
			{
				this->labelGPU->Text += "Issues: Unable to detect GPU architecture.\n(Click for more information)";
				GPUIssueText = "It looks like you have an NVIDIA GPU, but something went wrong while trying to determine your GPU's architecture (type).\nThis may be a caused by a bug, but it probably indicates potential issues.\nPlease make sure you have a GTX 600 series or later GPU. (GTX 400 series or later should also work)";
				this->labelGPU->LinkColor = System::Drawing::Color::Orange;
				showGpuDialog = true;
			}
			int linkEnd = this->labelGPU->Text->Length - linkStart;

			this->labelGPU->LinkClicked += gcnew System::Windows::Forms::LinkLabelLinkClickedEventHandler(this, &ui::LinkLabelLinkClickedGPUIssueHandler);
			this->labelGPU->LinkArea = System::Windows::Forms::LinkArea(linkStart, linkEnd);

			if (showGpuDialog && !nNoGPUDialog)
				SkinnedMessageBox::Show(this, GPUIssueText + "\n\nYou can disable this message from the options tab.", "PD Launcher", MessageBoxButtons::OK, MessageBoxIcon::Warning);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~ui()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^ button_Launch;
	private: System::Windows::Forms::Button^ button_Help;
	private: System::Windows::Forms::GroupBox^ groupBox_ScreenRes;
	private: System::Windows::Forms::TabControl^ tabControl;
	private: System::Windows::Forms::TabPage^ tabPage_Resolution;
	private: System::Windows::Forms::GroupBox^ groupBox_InternalRes;
	private: System::Windows::Forms::TabPage^ tabPage_Components;
	private: System::Windows::Forms::Panel^ panel_Components;
	private: System::Windows::Forms::TabPage^ tabPage_Patches;
	private: System::Windows::Forms::Panel^ panel_Patches;
	private: System::Windows::Forms::TabPage^ tabPage_Plugins;
	private: System::Windows::Forms::Panel^ panel_Plugins;
	private: System::Windows::Forms::Button^ button_Discord;
	private: System::Windows::Forms::Button^ button_github;
	private: System::Windows::Forms::TabPage^  tabPage_Playerdata;
	private: System::Windows::Forms::Panel^  panel_Playerdata;
	private: System::Windows::Forms::ToolTip^  toolTip1;
	private: System::Windows::Forms::Panel^  panel_ScreenRes;
	private: System::Windows::Forms::Panel^  panel_IntRes;
	private: System::Windows::Forms::LinkLabel^ labelGPU;
	private: System::Windows::Forms::Button^ button_Apply;














	private: System::ComponentModel::IContainer^ components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(ui::typeid));
			this->button_Launch = (gcnew System::Windows::Forms::Button());
			this->button_Help = (gcnew System::Windows::Forms::Button());
			this->groupBox_ScreenRes = (gcnew System::Windows::Forms::GroupBox());
			this->panel_ScreenRes = (gcnew System::Windows::Forms::Panel());
			this->tabControl = (gcnew System::Windows::Forms::TabControl());
			this->tabPage_Resolution = (gcnew System::Windows::Forms::TabPage());
			this->labelGPU = (gcnew System::Windows::Forms::LinkLabel());
			this->groupBox_InternalRes = (gcnew System::Windows::Forms::GroupBox());
			this->panel_IntRes = (gcnew System::Windows::Forms::Panel());
			this->tabPage_Patches = (gcnew System::Windows::Forms::TabPage());
			this->panel_Patches = (gcnew System::Windows::Forms::Panel());
			this->tabPage_Playerdata = (gcnew System::Windows::Forms::TabPage());
			this->panel_Playerdata = (gcnew System::Windows::Forms::Panel());
			this->tabPage_Components = (gcnew System::Windows::Forms::TabPage());
			this->panel_Components = (gcnew System::Windows::Forms::Panel());
			this->tabPage_Plugins = (gcnew System::Windows::Forms::TabPage());
			this->panel_Plugins = (gcnew System::Windows::Forms::Panel());
			this->button_Discord = (gcnew System::Windows::Forms::Button());
			this->button_github = (gcnew System::Windows::Forms::Button());
			this->toolTip1 = (gcnew System::Windows::Forms::ToolTip(this->components));
			this->button_Apply = (gcnew System::Windows::Forms::Button());
			this->groupBox_ScreenRes->SuspendLayout();
			this->tabControl->SuspendLayout();
			this->tabPage_Resolution->SuspendLayout();
			this->groupBox_InternalRes->SuspendLayout();
			this->tabPage_Patches->SuspendLayout();
			this->tabPage_Playerdata->SuspendLayout();
			this->tabPage_Components->SuspendLayout();
			this->tabPage_Plugins->SuspendLayout();
			this->SuspendLayout();
			// 
			// button_Launch
			// 
			this->button_Launch->FlatAppearance->BorderColor = System::Drawing::SystemColors::Control;
			this->button_Launch->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button_Launch->Location = System::Drawing::Point(3, 315);
			this->button_Launch->Name = L"button_Launch";
			this->button_Launch->Size = System::Drawing::Size(68, 23);
			this->button_Launch->TabIndex = 20;
			this->button_Launch->Text = L"Launch";
			this->button_Launch->Click += gcnew System::EventHandler(this, &ui::Button_Launch_Click);
			// 
			// button_Help
			// 
			this->button_Help->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->button_Help->FlatAppearance->BorderColor = System::Drawing::SystemColors::Control;
			this->button_Help->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button_Help->Location = System::Drawing::Point(76, 315);
			this->button_Help->Name = L"button_Help";
			this->button_Help->Size = System::Drawing::Size(69, 23);
			this->button_Help->TabIndex = 21;
			this->button_Help->Text = L"HELP ME";
			this->button_Help->Click += gcnew System::EventHandler(this, &ui::Button_Help_Click);
			// 
			// groupBox_ScreenRes
			// 
			this->groupBox_ScreenRes->Controls->Add(this->panel_ScreenRes);
			this->groupBox_ScreenRes->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->groupBox_ScreenRes->ForeColor = System::Drawing::Color::White;
			this->groupBox_ScreenRes->Location = System::Drawing::Point(5, 6);
			this->groupBox_ScreenRes->Name = L"groupBox_ScreenRes";
			this->groupBox_ScreenRes->Size = System::Drawing::Size(278, 113);
			this->groupBox_ScreenRes->TabIndex = 10;
			this->groupBox_ScreenRes->TabStop = false;
			this->groupBox_ScreenRes->Text = L"Screen Resolution";
			// 
			// panel_ScreenRes
			// 
			this->panel_ScreenRes->AutoScroll = true;
			this->panel_ScreenRes->Location = System::Drawing::Point(4, 19);
			this->panel_ScreenRes->Name = L"panel_ScreenRes";
			this->panel_ScreenRes->Size = System::Drawing::Size(269, 89);
			this->panel_ScreenRes->TabIndex = 0;
			// 
			// tabControl
			// 
			this->tabControl->Appearance = System::Windows::Forms::TabAppearance::FlatButtons;
			this->tabControl->Controls->Add(this->tabPage_Resolution);
			this->tabControl->Controls->Add(this->tabPage_Patches);
			this->tabControl->Controls->Add(this->tabPage_Playerdata);
			this->tabControl->Controls->Add(this->tabPage_Components);
			this->tabControl->Controls->Add(this->tabPage_Plugins);
			this->tabControl->Location = System::Drawing::Point(0, 0);
			this->tabControl->Name = L"tabControl";
			this->tabControl->SelectedIndex = 0;
			this->tabControl->Size = System::Drawing::Size(294, 310);
			this->tabControl->TabIndex = 10;
			// 
			// tabPage_Resolution
			// 
			this->tabPage_Resolution->BackColor = System::Drawing::Color::Transparent;
			this->tabPage_Resolution->BackgroundImage = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"tabPage_Resolution.BackgroundImage")));
			this->tabPage_Resolution->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			this->tabPage_Resolution->Controls->Add(this->labelGPU);
			this->tabPage_Resolution->Controls->Add(this->groupBox_InternalRes);
			this->tabPage_Resolution->Controls->Add(this->groupBox_ScreenRes);
			this->tabPage_Resolution->Location = System::Drawing::Point(4, 25);
			this->tabPage_Resolution->Name = L"tabPage_Resolution";
			this->tabPage_Resolution->Padding = System::Windows::Forms::Padding(3, 3, 3, 3);
			this->tabPage_Resolution->Size = System::Drawing::Size(286, 281);
			this->tabPage_Resolution->TabIndex = 0;
			this->tabPage_Resolution->Text = L"Resolution";
			// 
			// labelGPU
			// 
			this->labelGPU->AutoSize = true;
			this->labelGPU->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(80)), static_cast<System::Int32>(static_cast<System::Byte>(24)),
				static_cast<System::Int32>(static_cast<System::Byte>(24)), static_cast<System::Int32>(static_cast<System::Byte>(24)));
			this->labelGPU->Location = System::Drawing::Point(8, 209);
			this->labelGPU->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->labelGPU->MinimumSize = System::Drawing::Size(273, 13);
			this->labelGPU->Name = L"labelGPU";
			this->labelGPU->Size = System::Drawing::Size(273, 13);
			this->labelGPU->TabIndex = 21;
			this->labelGPU->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// groupBox_InternalRes
			// 
			this->groupBox_InternalRes->Controls->Add(this->panel_IntRes);
			this->groupBox_InternalRes->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->groupBox_InternalRes->ForeColor = System::Drawing::Color::White;
			this->groupBox_InternalRes->Location = System::Drawing::Point(5, 124);
			this->groupBox_InternalRes->Name = L"groupBox_InternalRes";
			this->groupBox_InternalRes->Size = System::Drawing::Size(278, 83);
			this->groupBox_InternalRes->TabIndex = 20;
			this->groupBox_InternalRes->TabStop = false;
			this->groupBox_InternalRes->Text = L"Internal Resolution";
			// 
			// panel_IntRes
			// 
			this->panel_IntRes->AutoScroll = true;
			this->panel_IntRes->Location = System::Drawing::Point(4, 19);
			this->panel_IntRes->Name = L"panel_IntRes";
			this->panel_IntRes->Size = System::Drawing::Size(269, 59);
			this->panel_IntRes->TabIndex = 1;
			// 
			// tabPage_Patches
			// 
			this->tabPage_Patches->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)),
				static_cast<System::Int32>(static_cast<System::Byte>(64)));
			this->tabPage_Patches->Controls->Add(this->panel_Patches);
			this->tabPage_Patches->Location = System::Drawing::Point(4, 25);
			this->tabPage_Patches->Name = L"tabPage_Patches";
			this->tabPage_Patches->Size = System::Drawing::Size(286, 281);
			this->tabPage_Patches->TabIndex = 1;
			this->tabPage_Patches->Text = L"Options";
			// 
			// panel_Patches
			// 
			this->panel_Patches->AutoScroll = true;
			this->panel_Patches->Location = System::Drawing::Point(0, 0);
			this->panel_Patches->Name = L"panel_Patches";
			this->panel_Patches->Size = System::Drawing::Size(286, 283);
			this->panel_Patches->TabIndex = 9;
			// 
			// tabPage_Playerdata
			// 
			this->tabPage_Playerdata->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)),
				static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)));
			this->tabPage_Playerdata->Controls->Add(this->panel_Playerdata);
			this->tabPage_Playerdata->Location = System::Drawing::Point(4, 25);
			this->tabPage_Playerdata->Name = L"tabPage_Playerdata";
			this->tabPage_Playerdata->Size = System::Drawing::Size(286, 281);
			this->tabPage_Playerdata->TabIndex = 3;
			this->tabPage_Playerdata->Text = L"Player";
			// 
			// panel_Playerdata
			// 
			this->panel_Playerdata->AutoScroll = true;
			this->panel_Playerdata->Location = System::Drawing::Point(0, 0);
			this->panel_Playerdata->Name = L"panel_Playerdata";
			this->panel_Playerdata->Size = System::Drawing::Size(286, 283);
			this->panel_Playerdata->TabIndex = 1;
			// 
			// tabPage_Components
			// 
			this->tabPage_Components->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)),
				static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)));
			this->tabPage_Components->Controls->Add(this->panel_Components);
			this->tabPage_Components->Location = System::Drawing::Point(4, 25);
			this->tabPage_Components->Name = L"tabPage_Components";
			this->tabPage_Components->Padding = System::Windows::Forms::Padding(3, 3, 3, 3);
			this->tabPage_Components->Size = System::Drawing::Size(286, 281);
			this->tabPage_Components->TabIndex = 2;
			this->tabPage_Components->Text = L"Components";
			// 
			// panel_Components
			// 
			this->panel_Components->AutoScroll = true;
			this->panel_Components->Location = System::Drawing::Point(0, 0);
			this->panel_Components->Name = L"panel_Components";
			this->panel_Components->Size = System::Drawing::Size(286, 283);
			this->panel_Components->TabIndex = 0;
			// 
			// tabPage_Plugins
			// 
			this->tabPage_Plugins->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)),
				static_cast<System::Int32>(static_cast<System::Byte>(64)));
			this->tabPage_Plugins->Controls->Add(this->panel_Plugins);
			this->tabPage_Plugins->Location = System::Drawing::Point(4, 25);
			this->tabPage_Plugins->Name = L"tabPage_Plugins";
			this->tabPage_Plugins->Padding = System::Windows::Forms::Padding(3, 3, 3, 3);
			this->tabPage_Plugins->Size = System::Drawing::Size(286, 281);
			this->tabPage_Plugins->TabIndex = 3;
			this->tabPage_Plugins->Text = L"Plugins";
			// 
			// panel_Plugins
			// 
			this->panel_Plugins->AutoScroll = true;
			this->panel_Plugins->Location = System::Drawing::Point(0, 0);
			this->panel_Plugins->Name = L"panel_Plugins";
			this->panel_Plugins->Size = System::Drawing::Size(286, 283);
			this->panel_Plugins->TabIndex = 0;
			// 
			// button_Discord
			// 
			this->button_Discord->BackColor = System::Drawing::Color::Transparent;
			this->button_Discord->FlatAppearance->BorderSize = 0;
			this->button_Discord->FlatAppearance->MouseDownBackColor = System::Drawing::Color::Transparent;
			this->button_Discord->FlatAppearance->MouseOverBackColor = System::Drawing::Color::Transparent;
			this->button_Discord->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button_Discord->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"button_Discord.Image")));
			this->button_Discord->Location = System::Drawing::Point(225, 311);
			this->button_Discord->Name = L"button_Discord";
			this->button_Discord->Size = System::Drawing::Size(32, 32);
			this->button_Discord->TabIndex = 31;
			this->button_Discord->UseVisualStyleBackColor = false;
			this->button_Discord->Click += gcnew System::EventHandler(this, &ui::button_Discord_Click);
			// 
			// button_github
			// 
			this->button_github->BackColor = System::Drawing::Color::Transparent;
			this->button_github->Cursor = System::Windows::Forms::Cursors::Hand;
			this->button_github->FlatAppearance->BorderSize = 0;
			this->button_github->FlatAppearance->MouseDownBackColor = System::Drawing::Color::Transparent;
			this->button_github->FlatAppearance->MouseOverBackColor = System::Drawing::Color::Transparent;
			this->button_github->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button_github->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"button_github.Image")));
			this->button_github->Location = System::Drawing::Point(262, 311);
			this->button_github->Name = L"button_github";
			this->button_github->Size = System::Drawing::Size(32, 32);
			this->button_github->TabIndex = 32;
			this->button_github->UseVisualStyleBackColor = false;
			this->button_github->Click += gcnew System::EventHandler(this, &ui::button_github_Click);
			// 
			// button_Apply
			// 
			this->button_Apply->FlatAppearance->BorderColor = System::Drawing::SystemColors::Control;
			this->button_Apply->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button_Apply->Location = System::Drawing::Point(150, 315);
			this->button_Apply->Name = L"button_Apply";
			this->button_Apply->Size = System::Drawing::Size(69, 23);
			this->button_Apply->TabIndex = 33;
			this->button_Apply->Text = L"Apply";
			this->button_Apply->Click += gcnew System::EventHandler(this, &ui::button_Apply_Click);
			// 
			// ui
			// 
			this->AcceptButton = this->button_Launch;
			this->AutoScaleDimensions = System::Drawing::SizeF(96, 96);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Dpi;
			this->AutoSize = true;
			this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)),
				static_cast<System::Int32>(static_cast<System::Byte>(64)));
			this->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			this->ClientSize = System::Drawing::Size(293, 343);
			this->Controls->Add(this->button_Apply);
			this->Controls->Add(this->tabControl);
			this->Controls->Add(this->button_Help);
			this->Controls->Add(this->button_Launch);
			this->Controls->Add(this->button_github);
			this->Controls->Add(this->button_Discord);
			this->DoubleBuffered = true;
			this->ForeColor = System::Drawing::Color::White;
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->KeyPreview = true;
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"ui";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"PD Launcher";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &ui::Ui_FormClosing);
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &ui::Ui_FormClosed);
			this->Load += gcnew System::EventHandler(this, &ui::Ui_Load);
			this->groupBox_ScreenRes->ResumeLayout(false);
			this->tabControl->ResumeLayout(false);
			this->tabPage_Resolution->ResumeLayout(false);
			this->tabPage_Resolution->PerformLayout();
			this->groupBox_InternalRes->ResumeLayout(false);
			this->tabPage_Patches->ResumeLayout(false);
			this->tabPage_Playerdata->ResumeLayout(false);
			this->tabPage_Components->ResumeLayout(false);
			this->tabPage_Plugins->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void SaveSettings() {
	if (*ResolutionConfigChanged)
	{
		for (ConfigOptionBase* option : screenResolutionArray)
		{
			option->SaveOption();
		}

		for (ConfigOptionBase* option : internalResolutionArray)
		{
			option->SaveOption();
		}

		*ResolutionConfigChanged = false;
	}

	if (*OptionsConfigChanged)
	{
		for (ConfigOptionBase* option : optionsArray)
		{
			option->SaveOption();
		}

		*OptionsConfigChanged = false;
	}

	if (*PlayerdataConfigChanged)
	{
		for (ConfigOptionBase* option : playerdataArray)
		{
			option->SaveOption();
		}

		*PlayerdataConfigChanged = false;
	}

	if (*ComponentsConfigChanged)
	{
		for (ConfigOptionBase* component : componentsArray)
		{
			component->SaveOption();
		}

		*ComponentsConfigChanged = false;
	}

	for (ConfigOptionBase* option : AllPluginOpts)
	{
		if (*(option->hasChanged))
		{
			option->SaveOption();
		}

		*(option->hasChanged) = false;
	}
}
private: System::Void Ui_Load(System::Object^ sender, System::EventArgs^ e){
}
private: System::Void Button_Help_Click(System::Object^ sender, System::EventArgs^ e) {
	System::Diagnostics::Process::Start("https://notabug.org/nastys/PD-Loader/wiki");
}
private: System::Void Button_Launch_Click(System::Object^ sender, System::EventArgs^ e) {

	SaveSettings();

	// read the command line here so it'll be up to date even if the user changed it
	WCHAR stringBuf[256];
	GetPrivateProfileStringW(LAUNCHER_SECTION, L"command_line", L"", stringBuf, 256, CONFIG_FILE);

	DIVA_EXECUTABLE_LAUNCH_STRING += L" " + wstring(stringBuf);
	DIVA_EXECUTABLE_LAUNCH = const_cast<WCHAR*>(DIVA_EXECUTABLE_LAUNCH_STRING.c_str());


	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	CreateProcessW(DIVA_EXECUTABLE, DIVA_EXECUTABLE_LAUNCH, NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	
	// this->Close won't work in here since it will prompt the user to save the settings
	TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
}
private: System::Void InternalResEnabledChangedHandler(System::Object^ sender, System::EventArgs^ e) {
	if (((CheckBox^)CheckBox::FromHandle(InternalResolutionCheckbox->mainControlHandle))->Checked)
	{
		((Control^)Control::FromHandle(InternalResolutionOption->mainControlHandle))->Enabled = true;
	}
	else
	{
		((Control^)Control::FromHandle(InternalResolutionOption->mainControlHandle))->Enabled = false;
	}
}
private: System::Void DisplayTypeChangedHandler(System::Object^ sender, System::EventArgs^ e) {
	int idx = ((ComboBox^)ComboBox::FromHandle(DisplayModeDropdown->mainControlHandle))->SelectedIndex;

	if (idx == 2) // fullscreen
	{
		for (ConfigOptionBase* option : screenResolutionArray)
		{
			if (option != DisplayModeDropdown && option != DisplayResolutionOption)
			{
				((Control^)Control::FromHandle(option->mainControlHandle))->Enabled = true;
			}
		}
	}
	else
	{
		for (ConfigOptionBase* option : screenResolutionArray)
		{
			if (option != DisplayModeDropdown && option != DisplayResolutionOption)
			{
				((Control^)Control::FromHandle(option->mainControlHandle))->Enabled = false;
			}
		}
	}
}
private: System::Void Ui_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e) {
	if (!AnyConfigChanged())
		return;

	switch (SkinnedMessageBox::Show(this, "Do you want to save your settings?", "PD Launcher", MessageBoxButtons::YesNoCancel, MessageBoxIcon::Question))
	{
	case System::Windows::Forms::DialogResult::Yes:
		SaveSettings();
		break;

	case System::Windows::Forms::DialogResult::No:
		break;

	case System::Windows::Forms::DialogResult::Cancel:
		e->Cancel = true;
		break;
	}
}
private: System::Void Ui_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e) {
	// Prevents abnormal termination messages, remember that the game is still technically running and must be killed!
	TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
}
private: System::Void button_Discord_Click(System::Object^ sender, System::EventArgs^ e) {
	System::Diagnostics::Process::Start("https://discord.gg/cvBVGDZ");
}
private: System::Void button_github_Click(System::Object^ sender, System::EventArgs^ e) {
	System::Diagnostics::Process::Start("https://notabug.org/nastys/PD-Loader");
}
private: System::Void button_Apply_Click(System::Object^ sender, System::EventArgs^ e) {
	SaveSettings();
}
private: System::Void Ui_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) {
	if (e->KeyCode == Keys::Escape) this->Close();
}

private: bool* ResolutionConfigChanged = new bool(false);
		 bool* OptionsConfigChanged = new bool(false);
		 bool* PlayerdataConfigChanged = new bool(false);
		 bool* ComponentsConfigChanged = new bool(false);

private: bool AnyConfigChanged() {
	if (*ResolutionConfigChanged || *OptionsConfigChanged || *PlayerdataConfigChanged || *ComponentsConfigChanged)
		return true; // fast case

	for (ConfigOptionBase* option : AllPluginOpts)
	{
		if (*(option->hasChanged)) return true; // slow case for iterating over individual plugins
	}

	return false;
}

private: String^ GPUIssueText;
private: System::Void LinkLabelLinkClickedGPUIssueHandler(System::Object^ sender, System::Windows::Forms::LinkLabelLinkClickedEventArgs^ e) {
	SkinnedMessageBox::Show(this, GPUIssueText, "PD Launcher", MessageBoxButtons::OK, MessageBoxIcon::Warning);
}
};
}
