using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EditorUI
{
    interface EditorWindow
    {
        void ParentRender(IntPtr child);
        void UpdateChildPosition();
    }
}
