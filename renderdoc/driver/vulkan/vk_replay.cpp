/******************************************************************************
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Baldur Karlsson
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include "vk_replay.h"
#include "vk_core.h"
#include "vk_resources.h"

#include "serialise/string_utils.h"

VulkanReplay::OutputWindow::OutputWindow() : wnd(NULL_WND_HANDLE), width(0), height(0),
	dsimg(VK_NULL_HANDLE), dsmem(VK_NULL_HANDLE)
{
	swap = VK_NULL_HANDLE;
	for(size_t i=0; i < ARRAY_COUNT(colimg); i++)
	{
		colimg[i] = VK_NULL_HANDLE;
		colview[i] = VK_NULL_HANDLE;
	}

	VkImageMemoryBarrier t = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL,
		0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
		0, 0, VK_NULL_HANDLE,
		{ VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 }
	};
	for(size_t i=0; i < ARRAY_COUNT(coltrans); i++)
		coltrans[i] = t;

	t.subresourceRange.aspect = VK_IMAGE_ASPECT_DEPTH;
	depthtrans = t;

	t.subresourceRange.aspect = VK_IMAGE_ASPECT_STENCIL;
	stenciltrans = t;
}

void VulkanReplay::OutputWindow::SetCol(VkDeviceMemory mem, VkImage img)
{
}

void VulkanReplay::OutputWindow::SetDS(VkDeviceMemory mem, VkImage img)
{
}

void VulkanReplay::OutputWindow::MakeTargets(const VulkanFunctions &vk, VkDevice device, bool depth)
{
	vk.vkDeviceWaitIdle(device);

	for(size_t i=0; i < ARRAY_COUNT(colimg); i++)
	{
		if(colimg[i] != VK_NULL_HANDLE)
		{
			vk.vkDestroyAttachmentView(device, colview[i]);
			colimg[i] = VK_NULL_HANDLE;
			colview[i] = VK_NULL_HANDLE;
		}
	}

	if(dsimg != VK_NULL_HANDLE)
	{
		vk.vkDestroyAttachmentView(device, dsview);
		vk.vkDestroyImage(device, dsimg);
		vk.vkFreeMemory(device, dsmem);

		dsview = VK_NULL_HANDLE;
		dsimg = VK_NULL_HANDLE;
		dsmem = VK_NULL_HANDLE;
	}

	VkSwapChainWSI old = swap;

	VkPlatformHandleXcbWSI handle;
	handle.connection = connection;
	handle.root = screen->root;

	VkSurfaceDescriptionWindowWSI surfDesc = { VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_WSI, NULL, VK_PLATFORM_X11_WSI, &handle, &wnd };

	VkSwapChainCreateInfoWSI swapInfo = {
			VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI, NULL, (VkSurfaceDescriptionWSI *)&surfDesc,
			2, VK_FORMAT_B8G8R8A8_UNORM, { width, height }, 0,
			VK_SURFACE_TRANSFORM_NONE_WSI, 1, VK_PRESENT_MODE_IMMEDIATE_WSI,
			old, true,
	};

	VkResult res = vk.vkCreateSwapChainWSI(device, &swapInfo, &swap);
	RDCASSERT(res == VK_SUCCESS);

	if(old != VK_NULL_HANDLE)
		vk.vkDestroySwapChainWSI(device, old);

	size_t sz;
	res = vk.vkGetSwapChainInfoWSI(device, swap, VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI, &sz, NULL);
	RDCASSERT(res == VK_SUCCESS);

	numImgs = sz/sizeof(VkSwapChainImagePropertiesWSI);

	VkSwapChainImagePropertiesWSI* imgs = new VkSwapChainImagePropertiesWSI[numImgs];
	res = vk.vkGetSwapChainInfoWSI(device, swap, VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI, &sz, imgs);
	RDCASSERT(res == VK_SUCCESS);

	for(size_t i=0; i < numImgs; i++)
	{
		colimg[i] = imgs[i].image;
		coltrans[i].image = imgs[i].image;
		coltrans[i].oldLayout = coltrans[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	if(depth)
	{
		VULKANNOTIMP("Allocating depth-stencil image");

		/*
		dsmem = mem;
		dsimg = img;
		depthtrans.image = stenciltrans.image = img;
		depthtrans.oldLayout = depthtrans.newLayout = 
			stenciltrans.oldLayout = stenciltrans.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		*/
	}

	for(uint32_t i=0; i < numImgs; i++)
	{
		if(colimg[i] != VK_NULL_HANDLE)
		{
			VkAttachmentViewCreateInfo info = {
				VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO, NULL,
				colimg[i], VK_FORMAT_B8G8R8A8_UNORM, 0, 0, 1,
				0 };

			vk.vkCreateAttachmentView(device, &info, &colview[i]);
		}
	}

	if(dsimg != VK_NULL_HANDLE)
	{
		VkAttachmentViewCreateInfo info = {
			VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO, NULL,
			dsimg, VK_FORMAT_D32_SFLOAT_S8_UINT, 0, 0, 1,
			0 };

		vk.vkCreateAttachmentView(device, &info, &dsview);
	}
}

VulkanReplay::VulkanReplay()
{
	m_pDriver = NULL;
	m_Proxy = false;

	m_OutputWinID = 1;
	m_ActiveWinID = 0;
	m_BindDepth = false;
}

void VulkanReplay::Shutdown()
{
	delete m_pDriver;
}

APIProperties VulkanReplay::GetAPIProperties()
{
	APIProperties ret;

	ret.pipelineType = ePipelineState_D3D11;
	ret.degraded = false;

	return ret;
}

void VulkanReplay::ReadLogInitialisation()
{
	m_pDriver->ReadLogInitialisation();
}

void VulkanReplay::ReplayLog(uint32_t frameID, uint32_t startEventID, uint32_t endEventID, ReplayLogType replayType)
{
	m_pDriver->ReplayLog(frameID, startEventID, endEventID, replayType);
}

ResourceId VulkanReplay::GetLiveID(ResourceId id)
{
	return m_pDriver->GetResourceManager()->GetLiveID(id);
}

void VulkanReplay::InitCallstackResolver()
{
	m_pDriver->GetSerialiser()->InitCallstackResolver();
}

bool VulkanReplay::HasCallstacks()
{
	return m_pDriver->GetSerialiser()->HasCallstacks();
}

Callstack::StackResolver *VulkanReplay::GetCallstackResolver()
{
	return m_pDriver->GetSerialiser()->GetCallstackResolver();
}

vector<FetchFrameRecord> VulkanReplay::GetFrameRecord()
{
	return m_pDriver->GetFrameRecord();
}

vector<DebugMessage> VulkanReplay::GetDebugMessages()
{
	VULKANNOTIMP("GetDebugMessages");
	return vector<DebugMessage>();
}

vector<ResourceId> VulkanReplay::GetTextures()
{
	VULKANNOTIMP("GetTextures");
	vector<ResourceId> texs;

	ResourceId id;
	VkImage fakeBBIm = VK_NULL_HANDLE;
	VkDeviceMemory fakeBBMem = VK_NULL_HANDLE;
	m_pDriver->GetFakeBB(id, fakeBBIm, fakeBBMem);

	texs.push_back(id);
	return texs;
}
	
vector<ResourceId> VulkanReplay::GetBuffers()
{
	VULKANNOTIMP("GetBuffers");
	return vector<ResourceId>();
}

void VulkanReplay::PickPixel(ResourceId texture, uint32_t x, uint32_t y, uint32_t sliceFace, uint32_t mip, uint32_t sample, float pixel[4])
{
	RDCUNIMPLEMENTED("PickPixel");
}

uint32_t VulkanReplay::PickVertex(uint32_t frameID, uint32_t eventID, MeshDisplay cfg, uint32_t x, uint32_t y)
{
	RDCUNIMPLEMENTED("PickVertex");
	return ~0U;
}

bool VulkanReplay::RenderTexture(TextureDisplay cfg)
{
	VULKANNOTIMP("RenderTexture");
	return false;
}
	
void VulkanReplay::RenderCheckerboard(Vec3f light, Vec3f dark)
{
	auto it = m_OutputWindows.find(m_ActiveWinID);
	if(m_ActiveWinID == 0 || it == m_OutputWindows.end())
		return;

	OutputWindow &outw = it->second;

	VULKANNOTIMP("RenderCheckerboard");

	VkDevice dev = m_pDriver->GetDev();
	VkCmdBuffer cmd = m_pDriver->GetCmd();
	VkQueue q = m_pDriver->GetQ();
	const VulkanFunctions &vk = m_pDriver->m_Real;

	VkCmdBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO, NULL, VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT | VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT };

	VkResult res = vk.vkBeginCommandBuffer(cmd, &beginInfo);
	
	outw.curcoltrans->newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false, 1, (void **)&outw.curcoltrans);
	outw.curcoltrans->oldLayout = outw.curcoltrans->newLayout;
	
	VkClearColorValue clearColor = { { RANDF(0.0f, 1.0f), RANDF(0.0f, 1.0f), RANDF(0.0f, 1.0f), 1.0f, } };
	vk.vkCmdClearColorImage(cmd, outw.colimg[outw.curidx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clearColor, 1, &outw.curcoltrans->subresourceRange);
	
	res = vk.vkEndCommandBuffer(cmd);

	res = vk.vkQueueSubmit(q, 1, &cmd, VK_NULL_HANDLE);
}
	
void VulkanReplay::RenderHighlightBox(float w, float h, float scale)
{
	RDCUNIMPLEMENTED("RenderHighlightBox");
}
	
ResourceId VulkanReplay::RenderOverlay(ResourceId texid, TextureDisplayOverlay overlay, uint32_t frameID, uint32_t eventID, const vector<uint32_t> &passEvents)
{
	RDCUNIMPLEMENTED("RenderOverlay");
	return ResourceId();
}
	
void VulkanReplay::RenderMesh(uint32_t frameID, uint32_t eventID, const vector<MeshFormat> &secondaryDraws, MeshDisplay cfg)
{
	RDCUNIMPLEMENTED("RenderMesh");
}

bool VulkanReplay::CheckResizeOutputWindow(uint64_t id)
{
	if(id == 0 || m_OutputWindows.find(id) == m_OutputWindows.end())
		return false;
	
	OutputWindow &outw = m_OutputWindows[id];

	if(outw.wnd == NULL_WND_HANDLE)
		return false;
	
	int32_t w, h;
	GetOutputWindowDimensions(id, w, h);

	if(w != outw.width || h != outw.height)
	{
		outw.width = w;
		outw.height = h;

		if(outw.width > 0 && outw.height > 0)
		{
			bool depth = (outw.dsimg != VK_NULL_HANDLE);

			outw.MakeTargets(m_pDriver->m_Real, m_pDriver->GetDev(), depth);
		}

		return true;
	}

	return false;
}

void VulkanReplay::BindOutputWindow(uint64_t id, bool depth)
{
	m_ActiveWinID = id;
	m_BindDepth = depth;
	
	auto it = m_OutputWindows.find(id);
	if(id == 0 || it == m_OutputWindows.end())
		return;

	OutputWindow &outw = it->second;

	VkDevice dev = m_pDriver->GetDev();
	VkCmdBuffer cmd = m_pDriver->GetCmd();
	VkQueue q = m_pDriver->GetQ();
	const VulkanFunctions &vk = m_pDriver->m_Real;
	
	VkSemaphore sem;
	VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT };

	vk.vkCreateSemaphore(dev, &semInfo, &sem);

	vk.vkAcquireNextImageWSI(dev, outw.swap, UINT64_MAX, sem, &outw.curidx);

	outw.curcoltrans = &outw.coltrans[outw.curidx];

	vk.vkQueueWaitSemaphore(q, sem);

	vk.vkDestroySemaphore(dev, sem);
}

void VulkanReplay::ClearOutputWindowColour(uint64_t id, float col[4])
{
	VULKANNOTIMP("ClearOutputWindowColour");

	// VKTODO: same as FlipOutputWindow but do a colour clear
	// ultimately these functions should push commands into a queue and there should be a
	// more explicit start/end render functions (similar to BindOutputWindow, so it
	// could start the command buffer, and an end function could end it and submit it)
}

void VulkanReplay::ClearOutputWindowDepth(uint64_t id, float depth, uint8_t stencil)
{
	VULKANNOTIMP("ClearOutputWindowDepth");

	// VKTODO: same as FlipOutputWindow but do a depth clear
}

void VulkanReplay::FlipOutputWindow(uint64_t id)
{
	VULKANNOTIMP("FlipOutputWindow");

	auto it = m_OutputWindows.find(id);
	if(id == 0 || it == m_OutputWindows.end())
		return;

	OutputWindow &outw = it->second;

	VkDevice dev = m_pDriver->GetDev();
	VkCmdBuffer cmd = m_pDriver->GetCmd();
	VkQueue q = m_pDriver->GetQ();
	const VulkanFunctions &vk = m_pDriver->m_Real;
	
	// copy fake backbuffer into actual backbuffer

	ResourceId resid;
	VkImage fakeBBIm = VK_NULL_HANDLE;
	VkDeviceMemory fakeBBMem = VK_NULL_HANDLE;
	m_pDriver->GetFakeBB(resid, fakeBBIm, fakeBBMem);

	VkImageMemoryBarrier fakeTrans = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL,
		0, 0, VK_IMAGE_LAYOUT_PRESENT_SOURCE_WSI, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,
		0, 0, fakeBBIm,
		{ VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 } };

	VkCmdBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO, NULL, VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT | VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT };

	VkResult res = vk.vkBeginCommandBuffer(cmd, &beginInfo);
	RDCASSERT(res == VK_SUCCESS);

	void *barrier = (void *)&fakeTrans;

	vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false, 1, &barrier);
	fakeTrans.oldLayout = fakeTrans.newLayout;

	outw.curcoltrans->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL;
	vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false, 1, (void **)&outw.curcoltrans);
	outw.curcoltrans->oldLayout = outw.curcoltrans->newLayout;

	VkImageCopy region = {
		{ VK_IMAGE_ASPECT_COLOR, 0, 0}, { 0, 0, 0 },
		{ VK_IMAGE_ASPECT_COLOR, 0, 0}, { 0, 0, 0 },
		{ RDCMIN(1280, outw.width), RDCMIN(720, outw.height), 1 },
	};
	vk.vkCmdCopyImage(cmd, fakeBBIm, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL, outw.colimg[outw.curidx], VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL, 1, &region);
	
	fakeTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SOURCE_WSI;
	vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false, 1, &barrier);
	
	outw.curcoltrans->newLayout = VK_IMAGE_LAYOUT_PRESENT_SOURCE_WSI;
	vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false, 1,(void **) &outw.curcoltrans);
	outw.curcoltrans->oldLayout = outw.curcoltrans->newLayout;

	vk.vkEndCommandBuffer(cmd);

	vk.vkQueueSubmit(q, 1, &cmd, VK_NULL_HANDLE);
	
	{
		VkPresentInfoWSI presentInfo = { VK_STRUCTURE_TYPE_QUEUE_PRESENT_INFO_WSI, NULL, 1, &outw.swap, &outw.curidx };

		vk.vkQueuePresentWSI(q, &presentInfo);

		vk.vkQueueWaitIdle(q);
	}

	vk.vkDeviceWaitIdle(dev);
}

void VulkanReplay::DestroyOutputWindow(uint64_t id)
{
	auto it = m_OutputWindows.find(id);
	if(id == 0 || it == m_OutputWindows.end())
		return;

	OutputWindow &outw = it->second;

	const VulkanFunctions &vk = m_pDriver->m_Real;
	VkDevice device = m_pDriver->GetDev();

	for(size_t i=0; i < ARRAY_COUNT(outw.colimg); i++)
	{
		if(outw.colimg[i] != VK_NULL_HANDLE)
		{
			vk.vkDestroyAttachmentView(device, outw.colview[i]);
		}
	}

	if(outw.dsimg != VK_NULL_HANDLE)
	{
		vk.vkDestroyAttachmentView(device, outw.dsview);
		vk.vkDestroyImage(device, outw.dsimg);
		vk.vkFreeMemory(device, outw.dsmem);
	}

	vk.vkDestroySwapChainWSI(device, outw.swap);

	m_OutputWindows.erase(it);
}
	
uint64_t VulkanReplay::MakeOutputWindow(void *wn, bool depth)
{
	uint64_t id = m_OutputWinID;
	m_OutputWinID++;

	m_OutputWindows[id].SetWindowHandle(wn);

	if(wn != NULL)
	{
		int32_t w, h;
		GetOutputWindowDimensions(id, w, h);

		m_OutputWindows[id].width = w;
		m_OutputWindows[id].height = h;
		
		m_OutputWindows[id].MakeTargets(m_pDriver->m_Real, m_pDriver->GetDev(), depth);
	}

	return id;
}

vector<byte> VulkanReplay::GetBufferData(ResourceId buff, uint32_t offset, uint32_t len)
{
	RDCUNIMPLEMENTED("GetBufferData");
	return vector<byte>();
}

bool VulkanReplay::IsRenderOutput(ResourceId id)
{
	RDCUNIMPLEMENTED("IsRenderOutput");
	return false;
}

void VulkanReplay::FileChanged()
{
}

FetchTexture VulkanReplay::GetTexture(ResourceId id)
{
	VULKANNOTIMP("GetTexture");

	FetchTexture ret;
	ret.arraysize = 1;
	ret.byteSize = 1280*720*4;
	ret.creationFlags = eTextureCreate_SwapBuffer|eTextureCreate_SRV|eTextureCreate_RTV;
	ret.cubemap = false;
	ret.customName = false;
	ret.depth = 1;
	ret.width = 1280;
	ret.height = 720;
	ret.dimension = 2;
	ret.ID = id;
	ret.mips = 1;
	ret.msQual = 0;
	ret.msSamp = 1;
	ret.name = "WSI Presentable Image";
	ret.numSubresources = 1;

	ret.format.compByteWidth = 1;
	ret.format.compCount = 4;
	ret.format.compType = eCompType_UNorm;
	ret.format.rawType = 0;
	ret.format.special = false;
	ret.format.specialFormat = eSpecial_Unknown;
	ret.format.srgbCorrected = false;
	ret.format.strname = "B8G8R8A8_UNORM";
	return ret;
}

FetchBuffer VulkanReplay::GetBuffer(ResourceId id)
{
	RDCUNIMPLEMENTED("GetBuffer");
	return FetchBuffer();
}

ShaderReflection *VulkanReplay::GetShader(ResourceId id)
{
	RDCUNIMPLEMENTED("GetShader");
	return NULL;
}

void VulkanReplay::SavePipelineState()
{
	VULKANNOTIMP("SavePipelineState");
}

void VulkanReplay::FillCBufferVariables(ResourceId shader, uint32_t cbufSlot, vector<ShaderVariable> &outvars, const vector<byte> &data)
{
	RDCUNIMPLEMENTED("FillCBufferVariables");
}

bool VulkanReplay::GetMinMax(ResourceId texid, uint32_t sliceFace, uint32_t mip, uint32_t sample, float *minval, float *maxval)
{
	RDCUNIMPLEMENTED("GetMinMax");
	return false;
}

bool VulkanReplay::GetHistogram(ResourceId texid, uint32_t sliceFace, uint32_t mip, uint32_t sample, float minval, float maxval, bool channels[4], vector<uint32_t> &histogram)
{
	RDCUNIMPLEMENTED("GetHistogram");
	return false;
}

void VulkanReplay::InitPostVSBuffers(uint32_t frameID, uint32_t eventID)
{
	VULKANNOTIMP("VulkanReplay::InitPostVSBuffers");
}

vector<EventUsage> VulkanReplay::GetUsage(ResourceId id)
{
	VULKANNOTIMP("GetUsage");
	return vector<EventUsage>();
}

void VulkanReplay::SetContextFilter(ResourceId id, uint32_t firstDefEv, uint32_t lastDefEv)
{
	RDCUNIMPLEMENTED("SetContextFilter");
}

void VulkanReplay::FreeTargetResource(ResourceId id)
{
	RDCUNIMPLEMENTED("FreeTargetResource");
}

void VulkanReplay::FreeCustomShader(ResourceId id)
{
	RDCUNIMPLEMENTED("FreeCustomShader");
}

MeshFormat VulkanReplay::GetPostVSBuffers(uint32_t frameID, uint32_t eventID, uint32_t instID, MeshDataStage stage)
{
	MeshFormat ret;
	RDCEraseEl(ret);

	VULKANNOTIMP("VulkanReplay::GetPostVSBuffers");

	return ret;
}

byte *VulkanReplay::GetTextureData(ResourceId tex, uint32_t arrayIdx, uint32_t mip, bool resolve, bool forceRGBA8unorm, float blackPoint, float whitePoint, size_t &dataSize)
{
	RDCUNIMPLEMENTED("GetTextureData");
	return NULL;
}

void VulkanReplay::ReplaceResource(ResourceId from, ResourceId to)
{
	RDCUNIMPLEMENTED("ReplaceResource");
}

void VulkanReplay::RemoveReplacement(ResourceId id)
{
	RDCUNIMPLEMENTED("RemoveReplacement");
}

vector<uint32_t> VulkanReplay::EnumerateCounters()
{
	VULKANNOTIMP("EnumerateCounters");
	return vector<uint32_t>();
}

void VulkanReplay::DescribeCounter(uint32_t counterID, CounterDescription &desc)
{
	RDCUNIMPLEMENTED("DescribeCounter");
}

vector<CounterResult> VulkanReplay::FetchCounters(uint32_t frameID, uint32_t minEventID, uint32_t maxEventID, const vector<uint32_t> &counters)
{
	RDCUNIMPLEMENTED("FetchCounters");
	return vector<CounterResult>();
}

void VulkanReplay::BuildTargetShader(string source, string entry, const uint32_t compileFlags, ShaderStageType type, ResourceId *id, string *errors)
{
	RDCUNIMPLEMENTED("BuildTargetShader");
}

void VulkanReplay::BuildCustomShader(string source, string entry, const uint32_t compileFlags, ShaderStageType type, ResourceId *id, string *errors)
{
	RDCUNIMPLEMENTED("BuildCustomShader");
}

vector<PixelModification> VulkanReplay::PixelHistory(uint32_t frameID, vector<EventUsage> events, ResourceId target, uint32_t x, uint32_t y, uint32_t slice, uint32_t mip, uint32_t sampleIdx)
{
	RDCUNIMPLEMENTED("VulkanReplay::PixelHistory");
	return vector<PixelModification>();
}

ShaderDebugTrace VulkanReplay::DebugVertex(uint32_t frameID, uint32_t eventID, uint32_t vertid, uint32_t instid, uint32_t idx, uint32_t instOffset, uint32_t vertOffset)
{
	RDCUNIMPLEMENTED("DebugVertex");
	return ShaderDebugTrace();
}

ShaderDebugTrace VulkanReplay::DebugPixel(uint32_t frameID, uint32_t eventID, uint32_t x, uint32_t y, uint32_t sample, uint32_t primitive)
{
	RDCUNIMPLEMENTED("DebugPixel");
	return ShaderDebugTrace();
}

ShaderDebugTrace VulkanReplay::DebugThread(uint32_t frameID, uint32_t eventID, uint32_t groupid[3], uint32_t threadid[3])
{
	RDCUNIMPLEMENTED("DebugThread");
	return ShaderDebugTrace();
}

ResourceId VulkanReplay::ApplyCustomShader(ResourceId shader, ResourceId texid, uint32_t mip)
{
	RDCUNIMPLEMENTED("ApplyCustomShader");
	return ResourceId();
}

ResourceId VulkanReplay::CreateProxyTexture( FetchTexture templateTex )
{
	RDCUNIMPLEMENTED("CreateProxyTexture");
	return ResourceId();
}

void VulkanReplay::SetProxyTextureData(ResourceId texid, uint32_t arrayIdx, uint32_t mip, byte *data, size_t dataSize)
{
	RDCUNIMPLEMENTED("SetProxyTextureData");
}

ResourceId VulkanReplay::CreateProxyBuffer(FetchBuffer templateBuf)
{
	RDCUNIMPLEMENTED("CreateProxyBuffer");
	return ResourceId();
}

void VulkanReplay::SetProxyBufferData(ResourceId bufid, byte *data, size_t dataSize)
{
	RDCUNIMPLEMENTED("SetProxyTextureData");
}

const VulkanFunctions &GetRealVKFunctions();

ReplayCreateStatus Vulkan_CreateReplayDevice(const char *logfile, IReplayDriver **driver)
{
	RDCDEBUG("Creating a VulkanReplay replay device");
	
#if defined(WIN32)
	bool loaded = Process::LoadLibrary("vulkan.dll");
#elif defined(__linux__)
	bool loaded = Process::LoadLibrary("libvulkan.so");
#else
#error "Unknown platform"
#endif
	if(!loaded)
	{
		RDCERR("Failed to load vulkan library");
		return eReplayCreate_APIInitFailed;
	}
	
	VkInitParams initParams;
	RDCDriver driverType = RDC_Vulkan;
	string driverName = "VulkanReplay";
	if(logfile)
		RenderDoc::Inst().FillInitParams(logfile, driverType, driverName, (RDCInitParams *)&initParams);
	
	if(initParams.SerialiseVersion != VkInitParams::VK_SERIALISE_VERSION)
	{
		RDCERR("Incompatible VulkanReplay serialise version, expected %d got %d", VkInitParams::VK_SERIALISE_VERSION, initParams.SerialiseVersion);
		return eReplayCreate_APIIncompatibleVersion;
	}
	
	WrappedVulkan *vk = new WrappedVulkan(GetRealVKFunctions(), logfile);
	vk->Initialise(initParams);
	
	RDCLOG("Created device.");
	VulkanReplay *replay = vk->GetReplay();
	replay->SetProxy(logfile == NULL);

	*driver = (IReplayDriver *)replay;

	return eReplayCreate_Success;
}

static DriverRegistration VkDriverRegistration(RDC_Vulkan, "Vulkan", &Vulkan_CreateReplayDevice);