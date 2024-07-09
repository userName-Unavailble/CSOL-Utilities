PATH = "C:/Users/Silve/Develop/CSOL24H/source/lua/"
---加载 LUA 源文件。
---@param file_name string 文件名（相对路径）。
---@return nil
function Load(file_name)
    if (type(file_name) == "string")
    then
        pcall(dofile, PATH .. file_name)
    end
end