-- waypoint_imgui_poc.lua
--
-- Lua+ImGui waypoint POC script template.
-- This is a scaffold placeholder for when x86 Lua + ImGui runtime is wired into the DLL.
--
-- Expected host API (planned):
--   waypoint.get_entries() -> array of { waypoint_id, category_id, name }
--   waypoint.refresh()
--   waypoint.travel(waypoint_id)
--   imgui.* standard immediate-mode bindings

local M = {}

function M.draw()
  -- Example target flow:
  -- imgui.Begin("Waypoint Lua/ImGui POC")
  -- if imgui.Button("Refresh") then waypoint.refresh() end
  -- local entries = waypoint.get_entries()
  -- for i, e in ipairs(entries) do
  --   if imgui.Selectable(string.format("[%d] %s", e.waypoint_id, e.name), false) then
  --     waypoint.travel(e.waypoint_id)
  --   end
  -- end
  -- imgui.End()
end

return M
