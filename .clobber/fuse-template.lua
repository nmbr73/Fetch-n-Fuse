--[[--/*

  <<<FUSE_NAME>>>.fuse

  Based on https://www.shadertoy.com/view/<<<SHADER_ID>>> a WebGL shader created by <<<SHADER_AUTHOR>>>.
  Converted to DCTL and embeddet into a Lua Fuse by <<<FUSE_AUTHOR>>> (<<<FUSE_AUTHOR_URL>>>).
  Place this file in your Fusion's and/or DaVinci Resolve's 'Fuses/' folder to use it.

*/--]]--




-- /*
local ShaderFuse = require("Shaderfuse/ShaderFuse")
ShaderFuse.init()



-- // ------------------------------------------------------------------------
-- // Registry declaration
-- // ------------------------------------------------------------------------

FuRegisterClass(ShaderFuse.FuRegister.Name, CT_SourceTool, {
  ShaderFuse.FuRegister.Attributes,

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

  ShaderFuse.begin_create()

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
    INP_MaxScale        = 3,
    INP_MinAllowed      = 0,
    INP_MaxAllowed      = 3,
    MBTNC_ShowBasicButton = true,
    MBTNC_StretchToFit  = false, --true,
    MBTNC_ShowToolTip   = true,
    { MBTNC_AddButton = "Canvas", MBTNCD_ButtonWidth = 4/16, },
    { MBTNC_AddButton = "Wrap",MBTNCD_ButtonWidth = 3/16, },
    { MBTNC_AddButton = "Duplicate", MBTNCD_ButtonWidth = 5/16, },
    { MBTNC_AddButton = "Mirror", MBTNCD_ButtonWidth = 4/16, }, 
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
    INP_Default        = 0,
    INP_Integer        = true,
    ICD_Width          = 1,
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
		INP_Integer         = true,
		INP_MinScale 		= 0,
		INP_MaxScale 		= 4096,
	})
	InHeight = self:AddInput("Height", "_Height", {
		LINKID_DataType 	= "Number",
		INPID_InputControl 	= "SliderControl",
		INP_Default 		= 1080,
		INP_Integer         = true,
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


  ShaderFuse.end_create()

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
