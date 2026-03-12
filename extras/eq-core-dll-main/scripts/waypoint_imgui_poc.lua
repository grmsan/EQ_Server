local M = {}
local selected_index = 1

function M.draw()
  imgui.set_next_window_size(520, 420, imgui.COND_FIRST_USE_EVER)
  local visible = imgui.begin("Waypoint Lua/ImGui POC")
  if not visible then
    imgui.end_window()
    return
  end

  local entries = waypoint.get_entries()

  if imgui.button("Refresh") then
    waypoint.refresh()
  end

  imgui.same_line()
  if imgui.button("Travel") then
    local selected = entries[selected_index]
    if selected then
      waypoint.travel(selected.waypoint_id)
    end
  end

  imgui.separator()

  if not waypoint.has_packet() then
    imgui.text("No waypoint packet loaded yet. Click Refresh.")
  else
    imgui.text(string.format("Entries: %d", #entries))
  end

  for i, entry in ipairs(entries) do
    local label = string.format("[%d] %s", entry.waypoint_id, entry.name)
    local is_selected = (i == selected_index)
    if imgui.selectable(label, is_selected) then
      selected_index = i
      waypoint.set_selected_index(i)
    end
  end

  imgui.end_window()
end

return M
