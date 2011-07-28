require("romloader_usb")
require("romloader_uart")
require("flasher")
require("muhkuh")
require("select_plugin")
require("mhash")

FLASHER_PATH = "../targets/"
FLASHER_PATH_ATMEGA = "../targets/250khz_atmega_"

-- write binary file into string
-- returns true or false, message
function writeBin(strName, bin)
	local f, msg = io.open(strName, "wb")
	if f then 
		f:write(bin)
		f:close()
		return true, string.format("%d bytes written to file %s", bin:len(), strName)
	else
		print("Failed to open file for writing")
		return false, msg
	end
end

-- get hex representation (no spaces) of a byte string
function getHexString(strBin)
	local strHex = ""
	for i=1, strBin:len() do
		strHex = strHex .. string.format("%02x", strBin:byte(i))
	end
	return strHex
end

--------------------------------------------------------------------------
-- callback/progress function
-- progress is only printed every 10%
--------------------------------------------------------------------------

local progress_last_cnt
local progress_last_max
local progress_percent_step = 10
local function progress(cnt,max)
	if progress_last_max == nil or progress_last_cnt == nil -- first call to progress
		or progress_last_max ~= max or cnt < progress_last_cnt -- first call to progress in a new up/download
		or ((cnt - progress_last_cnt)*100/max) >= progress_percent_step then -- current up/download
			progress_last_cnt = cnt
			progress_last_max = max
			print(string.format("%d%% (%d/%d)", cnt*100/max, cnt, max))
	end
	return true
end

-- unused
local function callback(a,b)
	print(string.format(">'%s'", a))
	return true
end


--------------------------------------------------------------------------
-- handle command line arguments
--------------------------------------------------------------------------

MODE_FLASH = 0
MODE_READ = 2
MODE_VERIFY = 3
MODE_HASH = 4
MODE_DETECT = 5

MODE_WRITE_FUSES = 11
MODE_WRITE_LOCK = 12

MODE_VERIFY_HASH = 42
MODE_INFO = 8
MODE_HELP = 255

strUsage = [==[
Usage: lua cli_flash.lua mode parameters

                   Mode        Parameters
flash a file:      flash       [plugin] device filename
read flash:        read        [plugin] device range filename
verify flash:      verify      [plugin] device [offset] filename
compute SHA1:      hash        [plugin] device range
verify using hash: verify_hash [plugin] device [offset] filename
write fuse bits:   write_fuses [plugin] device fuses
write lock bits:   write_lock  [plugin] device -lb value
get board info:    info
detect flash:      detect      [plugin] device
show this help:    -h

plugin: -p plugin_name
        example: -p romloader_usb_00_00
        
device: -b bus -u unit -cs chip_select
        example: -b 0 -u 0 -cs 0
        unit and chip select default to 0, 0
        ATmega: -b 2 -u 1 -cs 0/1

range: -s device_start_offset -l len
       the start offset defaults to 0.
       ATmega: use byte offsets
       
offset: -s device_start_offset
       offset in the flash device, defaults to 0
       
fuses: [-fl value] [-fh value] [-fe value]
       fl = fuse low bits
       fh = fuse high bits
       fe = extended fuse bits
       values are between 00 and ff
]==]


function printf(...) print(string.format(...)) end

argdefs = {
b  = {argkey = "iBus",              clkey ="-b",   name="bus number"},
u  = {argkey = "iUnit",             clkey ="-u",   name="unit number"},
cs = {argkey = "iChipSelect",       clkey ="-cs",  name="chip select number"},
p  = {argkey = "strPluginName",     clkey ="-p",   name="plugin name"},
s  = {argkey = "ulStartOffset",     clkey ="-s",   name="start offset"},
l  = {argkey = "ulLen",             clkey ="-l",   name="number of bytes to read"},
f  = {argkey = "strDataFileName",   clkey = "",    name="file name"},
fl = {argkey = "iFuseBitsLow",      clkey = "-fl", name="fuse bits low"},
fh = {argkey = "iFuseBitsHigh",     clkey = "-fh", name="fuse bits high"},
fe = {argkey = "iFuseBitsExt",      clkey = "-fe", name="extended fuse bits"},
lb = {argkey = "iLockBits",         clkey = "-lb", name="lock bits"},
}

requiredargs = {
[MODE_FLASH]        = {"b", "u", "cs", "f"},
[MODE_READ]         = {"b", "u", "cs", "f", "l"},
[MODE_VERIFY]       = {"b", "u", "cs", "f"},
[MODE_VERIFY_HASH]  = {"b", "u", "cs", "f"},
[MODE_HASH]         = {"b", "u", "cs"},
[MODE_DETECT]       = {"b", "u", "cs"},
[MODE_WRITE_FUSES]  = {"b", "u", "cs"},
[MODE_WRITE_LOCK]   = {"b", "u", "cs", "lb"},
[MODE_INFO]         = {},
[MODE_HELP]         = {}
}

function checkArgs(aArgs)
	-- get the list of required/optional args for mode
	local required = requiredargs[aArgs.iMode]
	if not required then
		print("unknown mode!")
		return false
	end
	
	local fOk = true
	
	--show_args(aArgs)
	for i, strKey in pairs(required) do
		local argdef = argdefs[strKey]
		--print(strKey, argdef.argkey)
		if not aArgs[argdef.argkey] then
			printf("please specify %s (%s)", argdef.name, argdef.clkey)
			fOk = false
		end
	end
		
	return fOk
end

-- Evaluate command line arguments.
-- returns list of arguments or nil and error message.
-- Entries returned in aArgs:
-- iMode              MODE_FLASH/MODE_READ/MODE_INFO/MODE_HELP
-- strPluginName      plugin name
-- iBus               bus number
-- iUnit              unit number 
-- iChipSelect        chip selet number
-- ulStartOffset      start offset for read
-- ulLen              number of bytes to read
-- strDataFileName    input/output file name


function evalArg()
	local aArgs = {}
	local iMode
	local iArg = 2
	local nArgs = #arg
	
	if nArgs>=2 and arg[1] == "flash" then
		aArgs.iMode = MODE_FLASH
	elseif nArgs>=2 and arg[1] == "read" then
		aArgs.iMode = MODE_READ
	elseif nArgs>=2 and arg[1] == "verify" then
		aArgs.iMode = MODE_VERIFY
	elseif nArgs>=2 and arg[1] == "verify_hash" then
		aArgs.iMode = MODE_VERIFY_HASH
	elseif nArgs>=2 and arg[1] == "hash" then
		aArgs.iMode = MODE_HASH
	elseif nArgs>=2 and arg[1] == "detect" then
		aArgs.iMode = MODE_DETECT
	elseif nArgs>=2 and arg[1] == "write_fuses" then
		aArgs.iMode = MODE_WRITE_FUSES
	elseif nArgs>=2 and arg[1] == "write_lock" then
		aArgs.iMode = MODE_WRITE_LOCK
	elseif nArgs>=1 and arg[1] == "info" then
		aArgs.iMode = MODE_INFO
	elseif nArgs>=1 and arg[1] == "-h" then
		aArgs.iMode = MODE_HELP
	else
		return nil
	end
	
	
	while (iArg <= nArgs) do
		local iRemArgs = nArgs - iArg + 1
		if arg[iArg] == "-p" and iRemArgs >=2 then
			aArgs.strPluginName = arg[iArg+1]
			iArg = iArg + 2
			
		elseif arg[iArg] == "-b" and iRemArgs >=2 then
			aArgs.iBus  = tonumber(arg[iArg+1])
			if not aArgs.iBus then
				return nil, "Error parsing bus number (-b)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-u" and iRemArgs >=2 then
			aArgs.iUnit  = tonumber(arg[iArg+1])
			if not aArgs.iUnit then
				return nil, "Error parsing unit number (-u)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-cs" and iRemArgs >=2 then
			aArgs.iChipSelect  = tonumber(arg[iArg+1])
			if not aArgs.iChipSelect then
				return nil, "Error parsing chip select number (-cs)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-s" and iRemArgs >=2 then
			aArgs.ulStartOffset  = tonumber(arg[iArg+1])
			if not aArgs.ulStartOffset then
				return nil, "Error parsing start offset (-s)"
			end
			iArg = iArg + 2

		elseif arg[iArg] == "-l" and iRemArgs >=2 then
			aArgs.ulLen  = tonumber(arg[iArg+1])
			if not aArgs.ulLen then
				return nil, "Error parsing length (-l)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-fl" and iRemArgs >=2 then
			local val = tonumber(arg[iArg+1])
			if val then
				if val>=0 and val<=255 then
					aArgs.iFuseBitsLow = val
				else
					return nil, "fuse bits low value (-fl) out of range"
				end
			else
				return nil, "Error parsing fuse bits low value (-fl)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-fh" and iRemArgs >=2 then
			local val = tonumber(arg[iArg+1])
			if val then
				if val>=0 and val<=255 then
					aArgs.iFuseBitsHigh = val
				else
					return nil, "fuse bits high value (-fh) out of range"
				end
			else
				return nil, "Error parsing fuse bits high value (-fh)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-fe" and iRemArgs >=2 then
			local val = tonumber(arg[iArg+1])
			if val then
				if val>=0 and val<=255 then
					aArgs.iFuseBitsExt = val
				else
					return nil, "ext. fuse bits value (-fe) out of range"
				end
			else
				return nil, "Error parsing ext. fuse bits value (-f�)"
			end
			iArg = iArg + 2
			
		elseif arg[iArg] == "-lb" and iRemArgs >=2 then
			local val = tonumber(arg[iArg+1])
			if val then
				if val>=0 and val<=255 then
					aArgs.iLockBits = val
				else
					return nil, "lock bits value (-lb) out of range"
				end
			else
				return nil, "Error parsing lock bits value (-lb)"
			end
			iArg = iArg + 2
			
		elseif iRemArgs == 1 then
			aArgs.strDataFileName = arg[iArg]
			iArg = iArg + 1
		else
			return nil, string.format("unknown argument: %s", arg[iArg])
		end
	end
		
	-- set defaults
	aArgs.iUnit = aArgs.iUnit or 0
	aArgs.iChipSelect = aArgs.iChipSelect or 0
	aArgs.ulStartOffset = aArgs.ulStartOffset or 0
	
	
	-- check for missing args
	if checkArgs(aArgs) then
		return aArgs
	else
		return nil
	end
	
	return aArgs
end

function show_args(aArgs)
	for k,v in pairs(aArgs) do 
		if type(v)=="number" then
			print(string.format("%-20s %d 0x%08x", k, v, v))
		else
			print(k,v)
		end
	end
end

--------------------------------------------------------------------------
-- get plugin
--------------------------------------------------------------------------

function getPluginByName(strName)
	print("getPluginByName")
	for iPluginClass, tPluginClass in ipairs(__MUHKUH_PLUGINS) do
		local iDetected
		local aDetectedInterfaces = {}
		print(string.format("Detecting interfaces with plugin %s", tPluginClass:GetID()))
		iDetected = tPluginClass:DetectInterfaces(aDetectedInterfaces)
		print(string.format("Found %d interfaces with plugin %s", iDetected, tPluginClass:GetID()))
		
		for i,v in ipairs(aDetectedInterfaces) do
			print(string.format("%d: %s (%s) Used: %s, Valid: %s", i, v:GetName(), v:GetTyp(), tostring(v:IsUsed()), tostring(v:IsValid())))
			if strName == v:GetName() then
				if not v:IsValid() then
					return nil, "Plugin is not valid"
				elseif v:IsUsed() then
					return nil, "Plugin is in use"
				else
					print("found plugin")
					local tPlugin = v:Create()
					if tPlugin then 
						tPlugin:Connect()
						return tPlugin
					else
						return nil, "Error creating plugin instance"
					end
				end
			end
		end
	end
	return nil, "plugin not found"
end




function getPlugin(strPluginName)
	local tPlugin, strError
	if strPluginName then
		-- get the plugin by name
		tPlugin, strError = getPluginByName(strPluginName)
	else
		-- Ask the user to pick a plugin.
		tPlugin = select_plugin.SelectPlugin(strPattern)
		if tPlugin then
			tPlugin:Connect()
		else
			strError = "No plugin selected!"
		end
	end
	
	return tPlugin, strError
end



--------------------------------------------------------------------------
-- flash
--   iBus
--   iUnit
--   iChipSelect
--   strDataFileName
--   ulStartOffset
--------------------------------------------------------------------------
function swapBytes(strData)
	print("swapBytes")
	local chars = {}
	for i=1, strData:len(), 2 do
		chars[i] = string.char(strData:byte(i+1))
		chars[i+1] = string.char(strData:byte(i))
	end
	return table.concat(chars)
end


function doFlash(tPlugin, args)
	local strDataFileName = args.strDataFileName
	local fOk
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end

	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end

	-- Load the data.
	print("Loading data file")
	strData, msg = muhkuh.load(strDataFileName)
	if not strData then
		return false, "Failed to load binary '" .. strDataFileName .. "': " .. msg
	end

	if args.iBus==2 then
		-- we don't have isErased and getEraseArea on ATMega
		local fIsErased = flasher.erase(tPlugin, aAttr, 0, 0xffffffff)
		if fIsErased~=true then
			return false, "Failed to erase the area!"
		end
		
		-- swap bytes in the data
		-- strData = swapBytes(strData)
		
	else
		-- Get the erase area.
		print("Erasing the area")
		local ulDataFirst, ulDataEnd, ulEraseFirst, ulEraseEnd
		ulDataFirst = args.ulStartOffset
		ulDataEnd = ulDataFirst + string.len(strData)
		ulEraseFirst,ulEraseEnd = flasher.getEraseArea(tPlugin, aAttr, ulDataFirst, ulDataEnd)
		print(string.format("Area:  [0x%08x, 0x%08x[", ulDataFirst, ulDataEnd))
		print(string.format("Erase: [0x%08x, 0x%08x[", ulEraseFirst, ulEraseEnd))
	
		-- Make sure the area is erased.
		-- TODO: for serial flashes:
		-- if the flash has erase-and-program capability, we might skip this
		local fIsErased = flasher.isErased(tPlugin, aAttr, ulEraseStart, ulEraseEnd)
		if fIsErased==nil then
			return false, "Failed to check if the area is erased!"
		elseif fIsErased==true then
			print("!ALL CLEAN! The area is erased.")
		else
			print("!!!DIRTY!!! The area must be erased before flashing!")
	
			--io.write("Press enter to start erasing >") io.read()
			
			fIsErased = flasher.erase(tPlugin, aAttr, ulEraseStart, ulEraseEnd)
			if fIsErased~=true then
				return false, "Failed to erase the area!"
			else
			
				--io.write("Press enter to start erase check >") io.read()
				fIsErased = flasher.isErased(tPlugin, aAttr, ulEraseStart, ulEraseEnd)
				if fIsErased~=true then
					return false, "The flasher pretended to erase the area, but it is still dirty!"
				end
			end
		end
	end

	--io.write("Press enter to start flashing >") io.read()
	-- Loop over the complete data array and flash it in chunks.
	
	-- Was ist, wenn Puffergr��e kein Vielfaches der Sektorgr��e ist,
	-- das Binary gr��er als der Puffer ist
	-- und das Flash kein PageErase/EraseAndProgram kann?
	
	local ulDataByteSize = strData:len()
	local ulDataOffset = 0
	local ulDeviceOffset = args.ulStartOffset
	while ulDataOffset<ulDataByteSize do
		-- Extract the next chunk.
		strChunk = strData:sub(ulDataOffset+1, ulDataOffset+aAttr.ulBufferLen)
		ulChunkSize = strChunk:len()

		-- Download the chunk to the buffer.
		tPlugin:write_image(aAttr.ulBufferAdr, strChunk, progress, ulChunkSize)

		-- Flash the chunk.
		print(string.format("flashing offset 0x%08x-0x%08x.", ulDeviceOffset, ulDeviceOffset+ulChunkSize))
		fOk = flasher.flash(tPlugin, aAttr, ulDeviceOffset, ulChunkSize, aAttr.ulBufferAdr)
		if not fOk then
			return false, "Failed to flash data!"
		end

		-- Increase pointers.
		ulDataOffset = ulDataOffset + ulChunkSize
		ulDeviceOffset = ulDeviceOffset + ulChunkSize
	end

	return true, "image flashed"
end


--------------------------------------------------------------------------
-- write fuse bits
--   iBus
--   iUnit
--   iChipSelect
--   iFuseBitsLow (optional)
--   iFuseBitsHigh (optional)
--   iFuseBitsExt (optional)
--------------------------------------------------------------------------

function doWriteFuses(tPlugin, args)
	local iFuseBitsLow = args.iFuseBitsLow
	local iFuseBitsHigh = args.iFuseBitsHigh
	local iFuseBitsExt = args.iFuseBitsExt
	local fOk
	
	-- checks
	if not iFuseBitsLow and not iFuseBitsHigh and not iFuseBitsExt then
		return false, "No fuses selected"
	end
	
	if args.iBus~=2 then
		return false, "Fuse bits are only supported on ATmega"
	end
		
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end

	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end

	-- write 
	local fOk = flasher.writeFuseBits(tPlugin, aAttr, iFuseBitsLow, iFuseBitsHigh, iFuseBitsExt)
	
	if not fOk then
		return false, "Failed to write fuse bits"
	else
		return true, "Fuse bits written"
	end
end

--------------------------------------------------------------------------
-- write lock bits
--   iBus
--   iUnit
--   iChipSelect
--   iLockBits 
--------------------------------------------------------------------------

function doWriteLockBits(tPlugin, args)
	local iLockBits = args.iLockBits
	local fOk
	
	if args.iBus~=2 then
		return false, "Lock bits are only supported on ATmega"
	end
		
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end

	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end

	-- write 
	local fOk = flasher.writeLockBits(tPlugin, aAttr, iLockBits)
	
	if not fOk then
		return false, "Failed to write lock bits"
	else
		return true, "Lock bits written"
	end
end

--------------------------------------------------------------------------
-- read
--   iBus
--   iUnit
--   iChipSelect
--   ulStartOffset
--   ulLen
--   strDataFileName
--------------------------------------------------------------------------

function doRead(tPlugin, args)
	local fOk, strMsg 
	local strDataFileName = args.strDataFileName
	local ulDeviceOffset = args.ulStartOffset
	local ulSize = args.ulLen
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end
	
	-- Verify the data in chunks.
	local ulBufferAddr = aAttr.ulBufferAdr
	local ulBufferLen = aAttr.ulBufferLen
	local astrChunks = {}
	while ulSize>0 do
		-- determine chunk size
		local ulChunkSize
		if ulSize > ulBufferLen then 
			ulChunkSize = ulBufferLen
		else
			ulChunkSize = ulSize 
		end
	
		-- Read chunk into buffer
		print(string.format("reading flash offset 0x%08x-0x%08x.", ulDeviceOffset, ulDeviceOffset+ulChunkSize))
		fOk = flasher.read(tPlugin, aAttr, ulDeviceOffset, ulDeviceOffset + ulChunkSize, ulBufferAddr)
		if not fOk then
			return false, "Error while reading from flash!"
		end
		
		-- Read the buffer 
		local strChunk = tPlugin:read_image(ulBufferAddr, ulChunkSize, progress, ulChunkSize)
		if not strChunk then
			return false, "Error while reading buffer!"
		end
	
		table.insert(astrChunks, strChunk)
		
		ulDeviceOffset = ulDeviceOffset + ulChunkSize
		ulSize = ulSize - ulChunkSize
	end

	local strBin = table.concat(astrChunks)
	fOk, strMsg = writeBin(args.strDataFileName, strBin)
	
	return fOk, strMsg
end



--------------------------------------------------------------------------
-- verify
--   iBus
--   iUnit
--   iChipSelect
--   ulStartOffset
--   strDataFileName
--------------------------------------------------------------------------

function doVerify(tPlugin, args)
	local fOk 
	local strDataFileName = args.strDataFileName
	local ulDeviceOffset = args.ulStartOffset
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end
	
	-- Load the data.
	print("Loading data file")
	local strData, msg = muhkuh.load(strDataFileName)
	if not strData then
		return false, "Failed to load binary '" .. strDataFileName .. "': " .. msg
	end
	
	local ulDataByteSize = strData:len()
	local ulDataOffset = 0

	while ulDataOffset<ulDataByteSize do
		-- Extract the next chunk.
		strChunk = strData:sub(ulDataOffset+1, ulDataOffset+aAttr.ulBufferLen)
		ulChunkSize = strChunk:len()

		-- Download the chunk to the buffer.
		tPlugin:write_image(aAttr.ulBufferAdr, strChunk, progress, ulChunkSize)

		-- Verify the chunk.
		print(string.format("verifying offset 0x%08x-0x%08x.", ulDeviceOffset, ulDeviceOffset+ulChunkSize))
		fOk = flasher.verify(tPlugin, aAttr, ulDeviceOffset, ulDeviceOffset + ulChunkSize, aAttr.ulBufferAdr)
		if not fOk then
			return false, "Differences were found!"
		end

		-- Increase pointers.
		ulDataOffset = ulDataOffset + ulChunkSize
		ulDeviceOffset = ulDeviceOffset + ulChunkSize
	end
	
	return true, "The data in flash is equal to the input file"
end



--------------------------------------------------------------------------
-- hash
--   iBus
--   iUnit
--   iChipSelect
--   ulStartOffset
--   ulLen
--------------------------------------------------------------------------

function doHash(tPlugin, args)
	local fOk 
	local ulDeviceOffset = args.ulStartOffset
	local ulLen = args.ulLen
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end
	
	local fOk, strHashBin = flasher.hash(tPlugin, aAttr, ulDeviceOffset, ulDeviceOffset + ulLen)
	if not fOk then
		return false, "Could not compute the checksum!"
	end
	
	local strSHA1 = getHexString(strHashBin)
	print("SHA1: " .. strSHA1)
	
	return true, strSHA1
end


--------------------------------------------------------------------------
-- verify_hash
--   iBus
--   iUnit
--   iChipSelect
--   ulStartOffset
--   strDataFileName
--------------------------------------------------------------------------


function doVerifyHash(tPlugin, args)
	local ulDeviceOffset = args.ulStartOffset
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- check if the selected flash is present
	print("Detecting flash device")
	fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end
	
	-- Load the data.
	print("Loading data file")
	strData, msg = muhkuh.load(args.strDataFileName)
	if not strData then
		return false, "Failed to load binary '" .. strDataFileName .. "': " .. msg
	end
	local ulDataByteSize = strData:len()
	
	-- compute the SHA1 of the data file
	local mh = mhash.mhash_state()
	mh:init(mhash.MHASH_SHA1)
	mh:hash(strData)
	local strFileHashBin = mh:hash_end()

	local strFileHash = getHexString(strFileHashBin)
	print("File SHA1: " .. strFileHash)
	
	
	-- compute the SHA1 of the data in the flash
	local fOk, strFlashHashBin = flasher.hash(tPlugin, aAttr, ulDeviceOffset, ulDeviceOffset + ulDataByteSize)
	if not fOk then
		return false, "Could not compute the checksum!"
	end
	
	local strFlashHash = getHexString(strFlashHashBin)
	print("Flash SHA1: " .. strFlashHash)
	print("File SHA1:  " .. strFileHash)

	-- compare
	if strFileHashBin == strFlashHashBin then
		print("Checksums are equal!")
		return true, "The data in the flash and the file have the same checksum"
	else
		print("Checksums are not equal!")
		return true, "The data in the flash and the file do not have the same checksum"
	end
end

--------------------------------------------------------------------------
-- detect
--   iBus
--   iUnit
--   iChipSelect
--------------------------------------------------------------------------

function doDetect(tPlugin, args)
	
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- check if the selected flash is present
	print("Detecting flash device")
	local fOk = flasher.detect(tPlugin, aAttr, args.iBus, args.iUnit, args.iChipSelect)
	if not fOk then
		return false, "Failed to get a device description!"
	end
	
	return true, "ok"
end



--------------------------------------------------------------------------
--  board info
--------------------------------------------------------------------------


function printobj(val, key, indent)
	key = key or ""
	indent = indent or ""
	
	if type(val)=="number" then
		print(string.format("%s%s = %d (number)", indent, key, val))
	elseif type(val)=="string" then
		print(string.format("%s%s = %s (string)", indent, key, val))
	elseif type(val)=="table" then
		local indent = indent .. "  "
		print(string.format("%s%s = {", indent, key))
		for k,v in pairs(val) do
			printobj(v, tostring(k), indent)
		end
		print(string.format("%s} -- %s", indent, key))
	end
end



function doInfo(tPlugin, aArgs)
	-- Download the flasher.
	print("Downloading flasher binary")
	local aAttr = flasher.download(tPlugin, FLASHER_PATH, progress)
	if not aAttr then
		return false, "Error while downloading flasher binary"
	end
	
	-- Get the board info table.
	aBoardInfo = flasher.getBoardInfo(tPlugin, aAttr)
	print("Board info:")
	for iBusCnt,tBusInfo in ipairs(aBoardInfo) do
		print(string.format("Bus %d:\t%s", tBusInfo.iIdx, tBusInfo.strName))
		if not tBusInfo.aUnitInfo then
			print("\tNo units.")
		else
			for iUnitCnt,tUnitInfo in ipairs(tBusInfo.aUnitInfo) do
				print(string.format("\tUnit %d:\t%s", tUnitInfo.iIdx, tUnitInfo.strName))
			end
		end
		print("")
	end
	
	-- printobj(aBoardInfo)
	return true
end



--------------------------------------------------------------------------
-- main
--------------------------------------------------------------------------

local args, strMsg = evalArg()

if args == nil then
	if strMsg then
		print(strMsg)
	end
	print(strUsage)
	os.exit(1)
	
elseif args.iMode == MODE_HELP then
	print(strUsage)
	os.exit(0)
	
else
	
	if args.iBus == flasher.BUS_Spi_ATMega then
		FLASHER_PATH = FLASHER_PATH_ATMEGA
	end
	
	show_args(args)
	local fOk = false
	local tPlugin
	tPlugin, strMsg = getPlugin(args.strPluginName)
	if tPlugin then
		if args.iMode == MODE_FLASH then
			fOk, strMsg = doFlash(tPlugin, args)
		elseif args.iMode == MODE_READ then
			fOk, strMsg = doRead(tPlugin, args)
		elseif args.iMode == MODE_VERIFY then
			fOk, strMsg = doVerify(tPlugin, args)
		elseif args.iMode == MODE_VERIFY_HASH then
			fOk, strMsg = doVerifyHash(tPlugin, args)
		elseif args.iMode == MODE_HASH then
			fOk, strMsg = doHash(tPlugin, args)
		elseif args.iMode == MODE_WRITE_FUSES then
			fOk, strMsg = doWriteFuses(tPlugin, args)
		elseif args.iMode == MODE_WRITE_LOCK then
			fOk, strMsg = doWriteLockBits(tPlugin, args)
		elseif args.iMode == MODE_DETECT then
			fOk, strMsg = doDetect(tPlugin, args)
		elseif args.iMode == MODE_INFO then
			fOk, strMsg = doInfo(tPlugin, args)
		end
		tPlugin:Disconnect()
		tPlugin = nil
	end
	
	if fOk then
		if strMsg then 
			print(strMsg)
		end
		os.exit(0)
	else
		print(strMsg or "an unknown error occurred")
		os.exit(1)
	end
end

