


function install()

  local f = nil
  local n = nil

  local pathseparator     = package.config:sub(1,1)
  local repositorypath    = debug.getinfo(2, "S").short_src:match("(.*[/\\])install%.lua")
        repositorypath    = repositorypath:gsub('/',pathseparator)
  f = io.open(repositorypath..'.clobber/Incubator.fuse', "r")


  if not f then return "failed to read fuse sourcecode" end
  local incubator_source = f:read("*all")
  f:close()

  incubator_source, n = string.gsub(incubator_source, '%s+local%s+REPOSITORYPATH%s*=%s*%[%[[^%]]*%]%]', '\n\n  local REPOSITORYPATH=[['..repositorypath..']]')

  if n<1 then return "failed substitute repo path in fuse code" end

  local fusepath          = fusion:MapPath("Fuses:/")
  f = io.open(fusepath.."Incubator_temp.fuse","w")

  if not f then return "failed to write fuse" end

  f:write(incubator_source)
  f:close()

  print("\nAn 'Incubator_temp.fuse' has now been created in '"..fusepath.."'. Do not edit this file - it's a temporary copy and can be ovewritten at any time without further warning. This Fuse is connected to your repository in '"..repositorypath.."'. Drag'n'drop 'install.lua' onto your fusion compositon again whenever the location of your repository changes. Please make sure to restart DaFusion before trying to use that Fuse!\n")

  return nil
end



install()
