--[[--/*

  <<<FUSE_NAME>>>.fuse

  Based on https://www.shadertoy.com/view/<<<SHADER_ID>>> a WebGL shader created by <<<SHADER_AUTHOR>>>.
  Converted to DCTL and embeddet into a Lua Fuse by <<<FUSE_AUTHOR>>> (<<<FUSE_AUTHOR_URL>>>).
  Place this file in your Fusion's and/or DaVinci Resolve's 'Fuses/' folder to use it.

*/--]]--




-- /*
-- MANDATORY -----------------------------------------------------------------
local shadertoy_name       = "<<<SHADER_NAME>>>"
local shadertoy_author     = "<<<SHADER_AUTHOR>>>"
local shadertoy_id         = "<<<SHADER_ID>>>"
local shadertoy_license    = ""
local dctlfuse_category    = "Incubator"
local dctlfuse_name        = "<<<FUSE_NAME>>>"
local dctlfuse_author      = "<<<FUSE_AUTHOR>>>"
-- OPTIONAL ------------------------------------------------------------------
local dctlfuse_versionNo   = 1
local dctlfuse_versionDate = "<<<FUSE_CREATION_DATE>>>"
local dctlfuse_authorurl   = "<<<FUSE_AUTHOR_URL>>>"
local dctlfuse_authorlogo  = '<<<FUSE_AUTHOR_LOGO>>>'



-- // ------------------------------------------------------------------------
-- // Registry declaration
-- // ------------------------------------------------------------------------

FuRegisterClass(

-- >>> SCHNIPP::FUREGISTERCLASS.version="MonumentsAndSites"
   (FC_PREFIX==nil and "ST_" or (FC_PREFIX=="" and "" or FC_PREFIX.."_"))..dctlfuse_name, CT_SourceTool, { REGS_Category = (FC_SUBMENU==nil and "Shadertoys (dev)" or (FC_SUBMENU=="" and "Fuses" or FC_SUBMENU))..(FC_CATEGORY==nil and "\\"..dctlfuse_category or ( FC_CATEGORY ~= "" and "\\"..FC_CATEGORY or "")), REGS_OpIconString = (FC_SCPREFIX==nil and "ST-" or (SC_SCPREFIX=="" and "" or FC_SCPREFIX.."-"))..shadertoy_id,
   REGS_OpDescription = "Shadertoy '"..shadertoy_name.."' (ID: "..shadertoy_id..") created by "..shadertoy_author.." and ported by "..dctlfuse_author..(shadertoy_license == "" and ". Copyright "..shadertoy_author.." (CC BY-NC-SA 3.0)" or ". "..shadertoy_license)..". This port is by no means meant to take advantage of anyone or to do anyone wrong : Contact us on Discord (https://discord.gg/75FUn4N4pv) and/or GitHub (https://github.com/nmbr73/Shadertoys) if you see your rights abused or your intellectual property violated by this work.",
   REG_Fuse_NoEdit = not(FC_DEVELOP==nil or FC_DEVELOP==true), REG_Fuse_NoReload = not(FC_DEVELOP==nil or FC_DEVELOP==true), REG_Fuse_TilePic = FC_TITLEPIC, REGS_Company = dctlfuse_company==nil and dctlfuse_author or dctlfuse_company, REGS_URL = dctlfuse_authorurl==nil and "https://nmbr73.github.io/Shadertoys/" or dctlfuse_authorurl,
   REG_Version = FC_VERSIONNO~=nil and FC_VERSIONNO or (dctlfuse_versionNo==nil and 1 or dctlfuse_versionNo),
-- <<< SCHNAPP::FUREGISTERCLASS

  REG_NoObjMatCtrls      = true,
  REG_NoMotionBlurCtrls  = true,
  REG_Source_GlobalCtrls = false,
  REG_Source_SizeCtrls   = true,
  REG_Source_AspectCtrls = true,
  REG_Source_DepthCtrls  = true,
  REG_OpNoMask           = true,
  REG_TimeVariant        = true,
  })



-- // ------------------------------------------------------------------------
-- // DCTL kernel parameters
-- // ------------------------------------------------------------------------

-- */
ShaderParameters =
[[
<<<CODE_PARAMETERS>>>
  int    width,height;
  int    compOrder;

]]
-- /*



-- // ------------------------------------------------------------------------
-- DCTL kernel compatibility code
-- // ------------------------------------------------------------------------

-- */
ShaderCompatibilityCode =
[[
<<<CODE_COMPATIBILITY>>>
]]
-- /*



-- // ------------------------------------------------------------------------
-- DCTL kernel implementation
-- // ------------------------------------------------------------------------

-- */
ShaderKernelCode =
[[
<<<CODE_KERNEL>>>
]]
-- /*



-- // ------------------------------------------------------------------------
-- // Create
-- // ------------------------------------------------------------------------

function Create()

  StandardShaderFuseTopControls();

  ----- Inspector Panel Controls

<<<CODE_CREATE>>>

  Sep3 = self:AddInput(string.rep("_", 52), "Separator3", {
    LINKID_DataType     = "Text",
    INPID_InputControl  = "LabelControl",
    INP_External        = false,
    INP_Passive         = true,
	  IC_Visible          = true,
    INP_DoNotifyChanged = true,
    IC_NoLabel          = true,
  })
  
  
  InEdges = self:AddInput("Edges", "Edges", {
    LINKID_DataType = "Number",
    INPID_InputControl  = "MultiButtonControl",
    INP_Default         = 3.0,
    INP_Integer         = true,
    INP_DoNotifyChanged = true,
    INP_External        = false,
    MBTNC_ForceButtons  = true,
    INP_MinScale        = 0,
    INP_MaxScale        = 4,
    INP_MinAllowed      = 0,
    INP_MaxAllowed      = 4,
    MBTNC_ShowBasicButton = true,
    MBTNC_StretchToFit  = false, --true,
    MBTNC_ShowToolTip   = true,
    { MBTNC_AddButton = "Canvas", MBTNCD_ButtonWidth = 4/16, },
    { MBTNC_AddButton = "Wrap",MBTNCD_ButtonWidth = 3/16, },
    { MBTNC_AddButton = "Duplicate", MBTNCD_ButtonWidth = 5/16, },
    { MBTNC_AddButton = "Mirror", MBTNCD_ButtonWidth = 4/16, }, 
    { MBTNC_AddButton = "No Normalized", MBTNCD_ButtonWidth = 5/16, }, 
  }) 

  InDebugImage = self:AddInput("DebugImage", "DebugImage", {
    LINKID_DataType = "Number",
    INPID_InputControl = "ComboControl",
    INP_Default = 0.0,
    INP_Integer = true,
    ICD_Width = 1,
    { CCS_AddString = "Final", },
    { CCS_AddString = "BufferA", },
    { CCS_AddString = "BufferB", },
    { CCS_AddString = "BufferC", },
    { CCS_AddString = "BufferD", },
    CC_LabelPosition = "Horizontal",
  })

  ----- Size & Depth
  InSize = self:AddInput("Size", "Size_Fuse", {
    LINKID_DataType  = "Number",
    INPID_InputControl = "ComboControl",
	  INP_DoNotifyChanged = true,
    INP_Default      = 0,
    INP_Integer      = true,
    ICD_Width        = 1,
	  { CCS_AddString  = "Default", },
    { CCS_AddString  = "Manually", },
	  { CCS_AddString  = "Image0", },
    { CCS_AddString  = "1920x1080", },
	  { CCS_AddString  = "1200x675", },
	  { CCS_AddString  = "800x450", },
	  { CCS_AddString  = "640x360", },
    CC_LabelPosition = "Horizontal",
	  ICS_ControlPage  = "Image",
  })
  
  InWidth = self:AddInput("Width", "_Width", {
		LINKID_DataType 	= "Number",
		INPID_InputControl 	= "SliderControl",
		INP_Default 		= 1920,
		INP_Integer     = true,
		INP_MinScale 		= 0,
		INP_MaxScale 		= 4096,
	})
	InHeight = self:AddInput("Height", "_Height", {
		LINKID_DataType 	= "Number",
		INPID_InputControl 	= "SliderControl",
		INP_Default 		= 1080,
		INP_Integer     = true,
		INP_MinScale 		= 0,
		INP_MaxScale 		= 4096,
	})
  
  InDepth = self:AddInput("Depth_Fuse", "Depth_Fuse", {
    LINKID_DataType  = "Number",
    INPID_InputControl = "ComboControl",
	  INP_DoNotifyChanged = true,
    INP_Default      = 0,
    INP_Integer      = true,
    ICD_Width        = 1,
    { CCS_AddString  = "Default", },
	  { CCS_AddString  = "int8", },
	  { CCS_AddString  = "int16", },
    { CCS_AddString  = "float16", },
    { CCS_AddString  = "float32", },
    CC_LabelPosition = "Horizontal",
	  ICS_ControlPage  = "Image",
  })
  
  InMyWidth = self:FindInput("Width")
	InMyWidth:SetAttrs({ IC_Visible = false })
	InMyHeight = self:FindInput("Height")
	InMyHeight:SetAttrs({ IC_Visible = false })
	InMyDepth = self:FindInput("Depth")
	InMyDepth:SetAttrs({ IC_Visible = false }) 

  ----- In/Out

<<<ICHANNELS_CREATE>>>
  OutImage = self:AddOutput("Output", "Output", {
    LINKID_DataType = "Image",
    LINK_Main       = 1,
  })


  StandardShaderFuseBottomControls();

end



-- // ------------------------------------------------------------------------
-- // Process
-- // ------------------------------------------------------------------------
function DefineEdges(edges, nodeX)

    --This gets the value of our input image for us to modify inside the kernel
    if edges == 0 then
      nodeX:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_BORDER, TEX_NORMALIZED_COORDS_TRUE)
    elseif edges == 1 then
      nodeX:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_WRAP, TEX_NORMALIZED_COORDS_TRUE)
    elseif edges == 2 then
      nodeX:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_DUPLICATE, TEX_NORMALIZED_COORDS_TRUE)
    elseif edges == 3 then
      nodeX:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    elseif edges == 4 then
      --print("Sampler 4")
    end
end



MULTIBUFFER = false
if MULTIBUFFER then   -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
  ImgAttrs_Global = {
          { IMG_Channel = "Red", },
          { IMG_Channel = "Green", },
          { IMG_Channel = "Blue", },
          { IMG_Channel = "Alpha", },
          IMG_Width = Width,
          IMG_Height = Height,
          IMG_DeferAlloc = false,
          }

  Image_Buff_GlobalA = Image(ImgAttrs_Global)
  Image_Buff_GlobalB = Image(ImgAttrs_Global)
  Image_Buff_GlobalC = Image(ImgAttrs_Global)
  Image_Buff_GlobalD = Image(ImgAttrs_Global)
end


function Process(req)

	-- Imagesize and Depth
  if (InSize:GetValue(req).Value >= 1) then
		if (InSize:GetValue(req).Value == 2) then
			if (InChannel0:GetValue(req) ~= nil) then
			   Width = InChannel0:GetValue(req).Width
			   Height = InChannel0:GetValue(req).Height
			end
		else
			Width = InWidth:GetValue(req).Value
			Height = InHeight:GetValue(req).Value 
		end
	end	
  
  -- Alle ( int und float )
  if (InDepth:GetValue(req).Value > 0) then
	  if InDepth:GetValue(req).Value == 1 then 
	    SourceDepth = 5 
    else 
	    if InDepth:GetValue(req).Value == 2 then 
	        SourceDepth = 6 
	    else 
	        if InDepth:GetValue(req).Value == 3 then 
 		        SourceDepth = 7 
		    	else
			      SourceDepth = 8
	        end
		  end
	  end
	end

  local imgattrs = {
    IMG_Document = self.Comp,
    { IMG_Channel = "Red", },
    { IMG_Channel = "Green", },
    { IMG_Channel = "Blue", },
    { IMG_Channel = "Alpha", },
    IMG_Width  = Width,
    IMG_Height = Height,
    IMG_XScale = XAspect,
    IMG_YScale = YAspect,
    IMAT_OriginalWidth  = realwidth, -- nil !?!
    IMAT_OriginalHeight = realheight, -- nil !?!
    IMG_Quality = not req:IsQuick(),
    IMG_MotionBlurQuality = not req:IsNoMotionBlur(),
    IMG_DeferAlloc = true,
    IMG_ProxyScale = ( (not req:IsStampOnly()) and 1 or nil),
    IMG_Depth = ( (SourceDepth~=0) and SourceDepth or nil   )
  }

  local dst   = Image(imgattrs)
  local black = Pixel({R=0,G=0,B=0,A=0})
  dst:Fill(black)

if MULTIBUFFER then -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
  dstA = Image {IMG_Like = dst, IMG_DeferAlloc = true}
	dstB = Image {IMG_Like = dst, IMG_DeferAlloc = true}
	dstC = Image {IMG_Like = dst, IMG_DeferAlloc = true}
	dstD = Image {IMG_Like = dst, IMG_DeferAlloc = true}
	dstI = Image {IMG_Like = dst, IMG_DeferAlloc = true}
end

  if req:IsPreCalc() then
    local out = Image({IMG_Like = dst, IMG_NoData = true})
    OutImage:Set(req, out)
    return
  end


if MULTIBUFFER then -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
    nodeA = DVIPComputeNode(req,
    "<<<FUSE_NAME>>>Fuse__Buffer_A", ShaderCompatibilityCode..ShaderKernelCode,
    "Params", ShaderParameters
  )
else
    node = DVIPComputeNode(req,
    "<<<FUSE_NAME>>>Fuse", ShaderCompatibilityCode..ShaderKernelCode,
    "Params", ShaderParameters
  )
end
  -- Extern texture or create a new one

<<<ICHANNELS_PROCESS1>>>
  -- DCTL parameters

  local framerate = self.Comp:GetPrefs("Comp.FrameFormat.Rate")

  local params = {}

  if MULTIBUFFER then -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
    params = nodeA:GetParamBlock(ShaderParameters)
  else  
    params = node:GetParamBlock(ShaderParameters)
  end  

<<<CODE_PROCESS>>>
  -- Resolution

  params.width  = dst.Width
  params.height = dst.Height

  -- Per channel time and resolution

<<<ICHANNELS_PROCESS2>>>

    local edges = InEdges:GetValue(req).Value

  -- Set parameters and add I/O
  if MULTIBUFFER then -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
    nodeA:SetParamBlock(params)
    --nodeA:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, nodeA)
    
    
  else   
    node:SetParamBlock(params)
    --node:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, node)
    
    <<<ICHANNELS_PROCESS3>>>
    node:AddOutput("dst", dst)
  end
  
  if MULTIBUFFER then  -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
    nodeA:AddInput("iChannel0",Image_Buff_GlobalC)  -- Anpassen !!
    nodeA:AddInput("iChannel1",Image_Buff_GlobalD)  -- Anpassen !!
    nodeA:AddOutput("dst", dstA)

    local ok = nodeA:RunSession(req)

    if (not ok) then
      dstA = nil
      dump(nodeA:GetErrorLog())
    end
    
    Image_Buff_GlobalA = dstA

  -------------------------- BufferB-Kernel----------------------------------------
    local nodeB = DVIPComputeNode(req,
      "<<<FUSE_NAME>>>Fuse__Buffer_B", ShaderCompatibilityCode..ShaderKernelCode,
      "Params", ShaderParameters
    )
    
    nodeB:SetParamBlock(params)

    --nodeB:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, nodeB)
    
    nodeB:AddInput("iChannel0", Image_Buff_GlobalA)  -- Anpassen !!
    nodeB:AddInput("iChannel1", Image_Buff_GlobalD)  -- Anpassen !!
    nodeB:AddOutput("dst", dstB)

    local success = nodeB:RunSession(req)
    if not success then
      dstB = nil
      dump(nodeB:GetErrorLog())
    end
    
    Image_Buff_GlobalB = dstB --Recursiv Image	
    

    -------------------------- BufferC-Kernel----------------------------------------
    local nodeC = DVIPComputeNode(req,
      "<<<FUSE_NAME>>>Fuse__Buffer_C", ShaderCompatibilityCode..ShaderKernelCode,
      "Params", ShaderParameters
    )
    
    nodeC:SetParamBlock(params)

    --nodeC:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, nodeC)

    nodeC:AddInput("iChannel0", Image_Buff_GlobalA)  -- Anpassen !!
    nodeC:AddInput("iChannel1", Image_Buff_GlobalB)  -- Anpassen !!
    nodeC:AddOutput("dst", dstC)

    local success = nodeC:RunSession(req)
    if not success then
      dstC = nil
      dump(nodeC:GetErrorLog())
    end
    
    Image_Buff_GlobalC = dstC --Recursiv Image	


    -------------------------- BufferD-Kernel----------------------------------------
    local nodeD = DVIPComputeNode(req,
      "<<<FUSE_NAME>>>Fuse__Buffer_D", ShaderCompatibilityCode..ShaderKernelCode,
      "Params", ShaderParameters
    )
    
    nodeD:SetParamBlock(params)

    --nodeD:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, nodeD)

    nodeD:AddInput("iChannel0", Image_Buff_GlobalC)  -- Anpassen !!
    nodeD:AddInput("iChannel1", Image_Buff_GlobalB)  -- Anpassen !!
    nodeD:AddOutput("dst", dstD)

    local success = nodeD:RunSession(req)
    if not success then
      dstD = nil
      dump(nodeD:GetErrorLog())
    end
    
    Image_Buff_GlobalD = dstD --Recursiv Image	


    -------------------------- ImageKernel----------------------------------------
    node = DVIPComputeNode(req,
      "<<<FUSE_NAME>>>Fuse", ShaderCompatibilityCode..ShaderKernelCode,
      "Params", ShaderParameters
    )

    node:SetParamBlock(params)
    --node:AddSampler("RowSampler", TEX_FILTER_MODE_LINEAR,TEX_ADDRESS_MODE_MIRROR, TEX_NORMALIZED_COORDS_TRUE)
    DefineEdges(edges, node)

    node:AddInput("iChannel0", Image_Buff_GlobalC)  -- Anpassen !!
    node:AddInput("iChannel1", iChannel0)           -- Anpassen !!
    node:AddOutput("dst", dst)
 
  end -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER 
  

  local ok = node:RunSession(req)

	if (not ok) then
		dst = nil
    dump(node:GetErrorLog())
	end

  OutImage:Set(req,dst)
  
  
    --Debugging
  if MULTIBUFFER then  -- MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER MULTIBUFFER
    InDebugImage:SetAttrs({ IC_Visible = true })
    
    if (InDebugImage:GetValue(req).Value == 1) then OutImage:Set(req, Image_Buff_GlobalA) end
    if (InDebugImage:GetValue(req).Value == 2) then OutImage:Set(req, Image_Buff_GlobalB) end
    if (InDebugImage:GetValue(req).Value == 3) then OutImage:Set(req, Image_Buff_GlobalC) end
    if (InDebugImage:GetValue(req).Value == 4) then OutImage:Set(req, Image_Buff_GlobalD) end
  else
    InDebugImage:SetAttrs({ IC_Visible = false })
  end
    
  
  collectgarbage();
end



-- // ------------------------------------------------------------------------
-- // Callback
-- // ------------------------------------------------------------------------

function NotifyChanged(inp, param, time)
 	if (param ~= nil) then

		if inp == InSize then
		  if param.Value == 1 then
			  InWidth:SetAttrs({ IC_Visible = true })
			  InHeight:SetAttrs({ IC_Visible = true })
		  else
			  InWidth:SetAttrs({ IC_Visible = false })
			  InHeight:SetAttrs({ IC_Visible = false })
		  end
		  
		  if param.Value == 3 then --1920x1080
			  InWidth:SetSource(Number(1920),0,0)
			  InHeight:SetSource(Number(1080),0,0)
		  end
		  if param.Value == 4 then --1200x675
			  InWidth:SetSource(Number(1200),0,0)
			  InHeight:SetSource(Number(675),0,0)
		  end
		  if param.Value == 5 then --800x450
			  InWidth:SetSource(Number(800),0,0)
			  InHeight:SetSource(Number(450),0,0)
		  end
 	    if param.Value == 6 then --640x360
		    InWidth:SetSource(Number(640),0,0)
		    InHeight:SetSource(Number(360),0,0)
		  end
		end 

 	end
end


-- */

-- /* ====================== DO NOT TOUCH OR APPEND ANY CODE HERE ===========================================

-- >>> SCHNIPP::SHADERFUSECONTROLS.version="MonumentsAndSites"

function ShaderFuseControls_AuthImg() if FC_AUTHIMG ~= nil and FC_AUTHIMG ~= ""  then self:AddInput('<p align="'..FC_AUTHIMGALIGN..'"><a href="https://github.com/nmbr73/Shadertoys"><img '..FC_AUTHIMG..' /></a></p>',"DctlFuseLogoLabel", {ICS_ControlPage = (FC_AUTHIMGPOS == 2 and 'Info' or nil),IC_ControlPage=(FC_AUTHIMGPOS == 2 and 1 or FC_AUTHIMGPOS),LINKID_DataType = "Text",INPID_InputControl = "LabelControl",LBLC_MultiLine = true,IC_NoLabel = true,IC_NoReset = true,INP_External = false,INP_Passive = true,}) end end
function ShaderFuseControls_InfoBtn() self:AddInput(dctlfuse_name.." Info ...", "DctlFuseInfoButton", {ICS_ControlPage = (FC_INFOBTNPOS == 2 and 'Info' or nil),IC_ControlPage = (FC_INFOBTNPOS == 2 and 1 or FC_INFOBTNPOS),LINKID_DataType = "Text",INPID_InputControl = "ButtonControl",INP_DoNotifyChanged = false,INP_External = false,BTNCS_Execute = 'bmd.openurl("'..(dctlfuse_infourl~=nil and dctlfuse_infourl or 'https://nmbr73.github.io/Shadertoys/Shaders/'..dctlfuse_category..'/'..dctlfuse_name..'.html')..'")'}) end
function ShaderFuseControls_BandImg() if FC_SHOWBRANDIMG then self:AddInput('<p align="center"><a href="https://github.com/nmbr73/Shadertoys"><img height="18" src="data:image/png;base64,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" /></a></p>', "DctlFuseBrandLabel",{ IC_ControlPage = -1 ,LINKID_DataType = "Text",INPID_InputControl = "LabelControl",LBLC_MultiLine = true,IC_NoLabel = true,IC_NoReset = true,INP_External = false,INP_Passive = true,}) end end

function ShaderFuseControls_InfoTxt()
  self:AddInput('<p align="'..FC_INFOTXTALIGN ..'">'
    ..'Shadertoy <a href="https://www.shadertoy.com/view/'..shadertoy_id..'" style="color:white; text-decoration:none; ">'..shadertoy_name..'</a> by <a href="https://www.shadertoy.com/user/'..shadertoy_author..'" style="color:yellow; text-decoration:none; ">'..shadertoy_author..'</a><br />'
    ..'<span style="color:#ff6060; ">'..(shadertoy_license == "" and '&copy; '..shadertoy_author..' (CC BY-NC-SA 3.0)' or shadertoy_license)..'</span><br />'
    ..'DCTLified and DaFused by <a href="'..(dctlfuse_authorurl==nil and "https://nmbr73.github.io/Shadertoys/" or dctlfuse_authorurl)..'" style="color:yellow; text-decoration:none; ">'..dctlfuse_author..'</a><br />'
    ..'<span style="color:#4060ff; ">'..(FC_VERSIONNO==nil and '' or 'Version '..FC_VERSIONNO)..(FC_VERSIONNO~=nil and FC_VERSIONDATE~=nil and ' - ' or '')..(FC_VERSIONDATE==nil and '' or FC_VERSIONDATE)..(FC_VERSIONNO~=nil and FC_VERSIONDATE~=nil and '<br />' or '')..'</span></p>'
    ,"DctlFuseInfoLabel", {ICS_ControlPage =(FC_INFOTXTPOS == 2 and 'Info' or nil),IC_ControlPage = (FC_INFOTXTPOS == 2 and 1 or FC_INFOTXTPOS),LINKID_DataType="Text",INPID_InputControl="LabelControl",LBLC_MultiLine=true,IC_NoLabel=true,IC_NoReset=true,INP_External=false,INP_Passive=true})
end

function StandardShaderFuseTopControls()
  if FC_ITEMORDER         == nil then FC_ITEMORDER       = {ShaderFuseControls_InfoTxt,ShaderFuseControls_BandImg,ShaderFuseControls_InfoBtn,ShaderFuseControls_AuthImg} end
  if FC_SHOWBRANDIMG      == nil then FC_SHOWBRANDIMG    = true                  end
  if FC_AUTHBASEDLAYOUT   == nil then FC_AUTHBASEDLAYOUT = true                  end
  if FC_INFOBTNPOS        == nil then FC_INFOBTNPOS      = 2                     end
  if FC_INFOTXTPOS        == nil then FC_INFOTXTPOS      = 2                     end
  if FC_INFOTXTALIGN      == nil then FC_INFOTXTALIGN    = 'center'              end
  if FC_AUTHIMGPOS        == nil then FC_AUTHIMGPOS      = 2                     end
  if FC_AUTHIMGALIGN      == nil then FC_AUTHIMGALIGN    = 'center'              end
  if FC_VERSIONNO         == nil then FC_VERSIONNO       = dctlfuse_versionNo    end
  if FC_VERSIONDATE       == nil then FC_VERSIONDATE     = dctlfuse_versionDate  end
  if FC_AUTHIMG           == nil then FC_AUTHIMG         = ''                    end
  if dctlfuse_authorlogo  ~= nil then FC_AUTHIMG         = dctlfuse_authorlogo   end
  if FC_AUTHBASEDLAYOUT then
    if dctlfuse_author=='JiPi' then
    --FC_SHOWBRANDIMG=false
      FC_ITEMORDER  = {ShaderFuseControls_BandImg, ShaderFuseControls_AuthImg,ShaderFuseControls_InfoBtn,ShaderFuseControls_InfoTxt}
      FC_INFOBTNPOS = 1; FC_AUTHIMGPOS = -1; FC_AUTHIMGALIGN = 'center'; FC_INFOTXTPOS = 1; FC_INFOTXTALIGN = 'center'
    elseif dctlfuse_author=='nmbr73' then
      FC_ITEMORDER = {ShaderFuseControls_InfoBtn,ShaderFuseControls_BandImg,ShaderFuseControls_InfoTxt,ShaderFuseControls_AuthImg}
      FC_INFOBTNPOS = -1; FC_AUTHIMGPOS = 1; FC_AUTHIMGALIGN = 'right'; FC_INFOTXTPOS = 1; FC_INFOTXTALIGN = 'left'
    end
  end
end

function StandardShaderFuseBottomControls()
  if FC_INFOBTNPOS == 1 or FC_INFOTXTPOS == 1 or FC_AUTHIMGPOS==1 then
    self:AddInput( '<br />',"DctlFuseSeparatorLabel",{LINKID_DataType="Text",INPID_InputControl="LabelControl",LBLC_MultiLine=true,IC_NoLabel=true,IC_NoReset=true,INP_External=false,INP_Passive=true})
  end
  FC_ITEMORDER[1](); FC_ITEMORDER[2](); FC_ITEMORDER[3](); FC_ITEMORDER[4]();
end
-- <<< SCHNAPP::SHADERFUSECONTROLS
-- */
