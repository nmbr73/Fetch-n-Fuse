


function install()

  local f = nil
  local n = nil

  local repositorypath    = debug.getinfo(2, "S").short_src:match("(.*[/\\])install%.lua")
  f = io.open(repositorypath..'.clobber/Incubator.fuse', "r")

  if not f then return "failed to read fuse sourcecode" end
  local incubator_source = f:read("*all")
  f:close()

  incubator_source, n = string.gsub(incubator_source, '%s+local%s+REPOSITORYPATH%s*=%s*"[^""]*"', '\n\nlocal REPOSITORYPATH="'..repositorypath..'"')

  if n<1 then return "failed substitute repo path in fuse code" end

  local fusepath          = fusion:MapPath("Fuses:/")
  f = io.open(fusepath.."IncubatorByNmbr73_JustDeleteIfYouWantTo.fuse","w")

  if not f then return "failed to write fuse" end

  f:write(incubator_source)
  f:close()


  return nil
end



install()
